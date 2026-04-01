#ifndef XFAWA_PARSER_H
#define XFAWA_PARSER_H

#include "xfawa_ast.h"
#include "xfawa_lexer.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

namespace xfawa {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::unordered_set<std::string> declaredVariables;
    
public:
    explicit Parser(const std::vector<Token>& toks);
    
    std::unique_ptr<Program> parseProgram();
    
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasWarnings() const { return !warnings.empty(); }
    const std::vector<std::string>& getWarnings() const { return warnings; }
    
private:
    const Token& peek() const;
    const Token& peek(int offset) const;
    Token consume();
    bool consume(TokenType type);
    bool isAtEnd() const;
    void advance();
    void addError(const std::string& message);
    void addWarning(const std::string& message);
    bool isVariableDeclared(const std::string& name) const;
    void declareVariable(const std::string& name);
    
    std::unique_ptr<Module> parseModule();
    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<PrintStatement> parsePrintStatement();
    std::unique_ptr<AssignmentStatement> parseAssignmentStatement();
    std::unique_ptr<AssignmentStatement> parseTypedAssignmentStatement(VarType type);
    std::unique_ptr<BreakStatement> parseBreakStatement();
    std::unique_ptr<ReturnStatement> parseReturnStatement();
    std::unique_ptr<BlockStatement> parseBlockStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<ImportStatement> parseImportStatement();
    std::unique_ptr<ForInStatement> parseForInStatement();
    
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseRelational();
    std::unique_ptr<Expression> parseAdditive();
    std::unique_ptr<Expression> parseMultiplicative();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parseArrayLiteral();
    std::unique_ptr<Expression> parsePostfix(std::unique_ptr<Expression> expr);
    
    static std::string statementTypeToString(Statement* stmt);
};

}

#endif
