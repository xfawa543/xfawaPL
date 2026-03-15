#ifndef XFAWA_SEMANTIC_ANALYZER_H
#define XFAWA_SEMANTIC_ANALYZER_H

#include "xfawa_ast.h"
#include "xfawa_namespace_policy.h"
#include "xfawa_error.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace xfawa {

struct FunctionInfo {
    std::string name;
    std::string ns;
    std::string moduleName;
    int paramCount;
    bool isPublic;
    SourceLocation location;
};

class SemanticAnalyzer {
private:
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, FunctionInfo> publicFunctions;
    std::unordered_map<std::string, FunctionInfo> privateFunctions;
    std::unordered_map<std::string, FunctionInfo> allFunctions;
    std::string currentModule;
    
public:
    bool analyze(Program* program) {
        if (!program) return false;
        
        for (const auto& mod : program->modules) {
            collectModuleFunctions(mod.get());
        }
        
        for (const auto& mod : program->modules) {
            if (!analyzeModule(mod.get())) {
                return false;
            }
        }
        
        return !hasErrors();
    }
    
    bool hasErrors() const { return !errors.empty(); }
    bool hasWarnings() const { return !warnings.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    const std::vector<std::string>& getWarnings() const { return warnings; }
    
private:
    void collectModuleFunctions(Module* mod) {
        if (!mod) return;
        
        for (const auto& func : mod->functions) {
            std::string key = func->ns.empty() ? 
                (mod->name + ":" + func->name) : 
                (func->ns + ":" + func->name);
            
            FunctionInfo info;
            info.name = func->name;
            info.ns = func->ns;
            info.moduleName = mod->name;
            info.paramCount = static_cast<int>(func->params.size());
            info.isPublic = !func->ns.empty();
            info.location = func->location;
            
            allFunctions[key] = info;
            
            if (info.isPublic) {
                publicFunctions[key] = info;
            } else {
                privateFunctions[key] = info;
            }
        }
    }
    bool analyzeModule(Module* mod) {
        if (!mod) return false;
        
        currentModule = mod->name;
        
        for (const auto& func : mod->functions) {
            if (!analyzeFunction(func.get())) {
                return false;
            }
        }
        
        return true;
    }
    
    bool analyzeFunction(Function* func) {
        if (!func) return false;
        
        if (!func->ns.empty()) {
            if (NamespacePolicy::isReserved(func->ns)) {
                errors.push_back(NamespacePolicy::getReservedNamespacesError(func->ns) + 
                    " at line " + std::to_string(func->location.line));
                return false;
            }
            
            if (!NamespacePolicy::isPublic(func->ns)) {
                errors.push_back(NamespacePolicy::getInvalidNamespaceError(func->ns) + 
                    " at line " + std::to_string(func->location.line));
                return false;
            }
        }
        
        std::string key = func->ns.empty() ? 
            (currentModule + ":" + func->name) : 
            (func->ns + ":" + func->name);
        
        FunctionInfo info;
        info.name = func->name;
        info.ns = func->ns;
        info.moduleName = currentModule;
        info.paramCount = static_cast<int>(func->params.size());
        info.isPublic = !func->ns.empty();
        info.location = func->location;
        
        if (info.isPublic) {
            if (publicFunctions.find(key) != publicFunctions.end()) {
                errors.push_back("Duplicate public function '" + func->ns + ":" + func->name + 
                    "' at line " + std::to_string(func->location.line));
                return false;
            }
            publicFunctions[key] = info;
        } else {
            privateFunctions[key] = info;
        }
        
        if (func->body) {
            if (!analyzeBlock(func->body.get())) {
                return false;
            }
        }
        
        return true;
    }
    
    bool analyzeBlock(BlockStatement* block) {
        if (!block) return true;
        
        for (const auto& stmt : block->statements) {
            if (!analyzeStatement(stmt.get())) {
                return false;
            }
        }
        
        return true;
    }
    
    bool analyzeStatement(Statement* stmt) {
        if (!stmt) return true;
        
        switch (stmt->getNodeType()) {
            case NodeType::PRINT_STATEMENT: {
                auto* printStmt = dynamic_cast<PrintStatement*>(stmt);
                return analyzeExpression(printStmt->expr.get());
            }
            case NodeType::EXPRESSION_STATEMENT: {
                auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmt);
                return analyzeExpression(exprStmt->expr.get());
            }
            case NodeType::ASSIGNMENT_STATEMENT: {
                auto* assignStmt = dynamic_cast<AssignmentStatement*>(stmt);
                return analyzeExpression(assignStmt->value.get());
            }
            case NodeType::RETURN_STATEMENT: {
                auto* retStmt = dynamic_cast<ReturnStatement*>(stmt);
                return retStmt->value ? analyzeExpression(retStmt->value.get()) : true;
            }
            case NodeType::BLOCK_STATEMENT: {
                auto* blockStmt = dynamic_cast<BlockStatement*>(stmt);
                return analyzeBlock(blockStmt);
            }
            case NodeType::WHILE_STATEMENT: {
                auto* whileStmt = dynamic_cast<WhileStatement*>(stmt);
                if (!analyzeExpression(whileStmt->condition.get())) return false;
                return analyzeStatement(whileStmt->body.get());
            }
            case NodeType::IF_STATEMENT: {
                auto* ifStmt = dynamic_cast<IfStatement*>(stmt);
                if (!analyzeExpression(ifStmt->condition.get())) return false;
                if (!analyzeStatement(ifStmt->thenBranch.get())) return false;
                for (const auto& elseIf : ifStmt->elseIfBranches) {
                    if (!analyzeExpression(elseIf.first.get())) return false;
                    if (!analyzeStatement(elseIf.second.get())) return false;
                }
                if (ifStmt->elseBranch && !analyzeStatement(ifStmt->elseBranch.get())) return false;
                return true;
            }
            case NodeType::FUNCTION_DECLARATION: {
                auto* funcDecl = dynamic_cast<FunctionDeclarationStatement*>(stmt);
                if (funcDecl->func) {
                    return analyzeFunction(funcDecl->func.get());
                }
                return true;
            }
            default:
                return true;
        }
    }
    
    bool analyzeExpression(Expression* expr) {
        if (!expr) return true;
        
        switch (expr->getNodeType()) {
            case NodeType::CALL_EXPRESSION: {
                auto* callExpr = dynamic_cast<CallExpression*>(expr);
                return analyzeCallExpression(callExpr);
            }
            case NodeType::BINARY_OP: {
                auto* binOp = dynamic_cast<BinaryOp*>(expr);
                if (!analyzeExpression(binOp->left.get())) return false;
                return analyzeExpression(binOp->right.get());
            }
            case NodeType::UNARY_OP: {
                auto* unaryOp = dynamic_cast<UnaryOp*>(expr);
                return analyzeExpression(unaryOp->expr.get());
            }
            default:
                return true;
        }
    }
    
    bool analyzeCallExpression(CallExpression* call) {
        if (!call) return true;
        
        if (call->name == "rnd") {
            if (call->args.size() != 1 && call->args.size() != 2) {
                errors.push_back("rnd() requires 1 (array) or 2 (min, max) arguments at line " + 
                    std::to_string(call->location.line));
                return false;
            }
            for (const auto& arg : call->args) {
                if (!analyzeExpression(arg.get())) {
                    return false;
                }
            }
            return true;
        }
        
        if (!call->ns.empty()) {
            if (NamespacePolicy::isReserved(call->ns)) {
                errors.push_back(NamespacePolicy::getReservedNamespacesError(call->ns) + 
                    " at line " + std::to_string(call->location.line));
                return false;
            }
            
            if (!NamespacePolicy::isPublic(call->ns)) {
                errors.push_back(NamespacePolicy::getInvalidNamespaceError(call->ns) + 
                    " at line " + std::to_string(call->location.line));
                return false;
            }
            
            std::string key = call->ns + ":" + call->name;
            if (publicFunctions.find(key) == publicFunctions.end()) {
                errors.push_back("Undefined public function: " + key + 
                    " at line " + std::to_string(call->location.line));
                return false;
            }
        } else {
            std::string key = currentModule + ":" + call->name;
            if (privateFunctions.find(key) == privateFunctions.end()) {
                bool found = false;
                for (const auto& pair : allFunctions) {
                    if (pair.second.name == call->name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    errors.push_back("Undefined function: " + call->name + 
                        " at line " + std::to_string(call->location.line));
                    return false;
                }
            }
        }
        
        for (const auto& arg : call->args) {
            if (!analyzeExpression(arg.get())) {
                return false;
            }
        }
        
        return true;
    }
};

}

#endif
