#ifndef XFAWA_LLVM_CODEGEN_H
#define XFAWA_LLVM_CODEGEN_H

#include "xfawa_ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/ELFObjectFile.h>
#include <map>
#include <vector>
#include <string>

namespace xfawa {

class LLVMCodegen {
private:
    llvm::LLVMContext& context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    std::map<std::string, llvm::AllocaInst*> locals;
    std::vector<std::string> errors;
    bool hasMainFunction;
    llvm::BasicBlock* loopEndBB;
    
public:
    LLVMCodegen(llvm::LLVMContext& ctx, llvm::Module* mod);
    
    static void initializeTargets();
    
    bool codegenProgram(Program* program);
    
    bool emitObjectFile(const std::string& filename);
    bool emitObjectFile(const std::string& filename, bool keepLL, bool emitAsm, 
                        const std::string& llOutputPath, const std::string& asmOutputPath);
    bool linkExecutable(const std::string& objFile, const std::string& outFile);
    bool verifyModule();
    
    const std::vector<std::string>& getErrors() const { return errors; }
    
private:
    void initBuiltins();
    void addError(const std::string& message) { errors.push_back(message); }
    
    llvm::Value* codegen(Expression* expr);
    bool codegen(Statement* stmt);
    bool codegen(Module* mod);
    bool codegen(Function* func);
    bool codegen(ImportStatement* stmt);
    
    llvm::Value* codegen(NumberLiteral* expr);
    llvm::Value* codegen(BooleanLiteral* expr);
    llvm::Value* codegen(StringLiteral* expr);
    llvm::Value* codegen(VariableExpression* expr);
    llvm::Value* codegen(UnaryOp* expr);
    llvm::Value* codegen(BinaryOp* expr);
    llvm::Value* codegen(CallExpression* expr);
    llvm::Value* codegen(ArrayRangeExpression* expr);
    llvm::Value* codegen(ExpressionStatement* stmt);
    llvm::Value* codegen(AssignmentStatement* stmt);
    llvm::Value* codegen(PrintStatement* stmt);
    llvm::Value* codegen(ReturnStatement* stmt);
    llvm::Value* codegen(BreakStatement* stmt);
    llvm::Value* codegen(BlockStatement* stmt);
    llvm::Value* codegen(IfStatement* stmt);
    llvm::Value* codegen(WhileStatement* stmt);
};

}

#endif