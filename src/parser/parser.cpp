#include "xfawa_parser.h"
#include <sstream>
#include <algorithm>

namespace xfawa {

namespace {

bool isKeywordLikeNameToken(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER:
        case TokenType::KEYWORD_FN:
        case TokenType::KEYWORD_IF:
        case TokenType::KEYWORD_ELSE:
        case TokenType::KEYWORD_WHILE:
        case TokenType::KEYWORD_BREAK:
        case TokenType::KEYWORD_RETURN:
        case TokenType::KEYWORD_TRUE:
        case TokenType::KEYWORD_FALSE:
        case TokenType::KEYWORD_PRINT:
        case TokenType::KEYWORD_IMPORT:
        case TokenType::KEYWORD_INT:
        case TokenType::KEYWORD_FLOAT:
        case TokenType::KEYWORD_BOOL:
        case TokenType::KEYWORD_FOR:
        case TokenType::KEYWORD_WINDOW:
            return true;
        default:
            return false;
    }
}

}

Parser::Parser(const std::vector<Token>& toks) : tokens(toks), current(0) {}

const Token& Parser::peek() const {
    if (current < tokens.size()) {
        return tokens[current];
    }
    return tokens.back();
}

const Token& Parser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    return tokens.back();
}

Token Parser::consume() {
    if (current < tokens.size()) {
        return tokens[current++];
    }
    return tokens.back();
}

bool Parser::consume(TokenType type) {
    if (peek().is(type)) {
        current++;
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return peek().is(TokenType::END_OF_FILE);
}

void Parser::advance() {
    if (current < tokens.size()) {
        current++;
    }
}

void Parser::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(peek().location.line));
}

void Parser::addWarning(const std::string& message) {
    warnings.push_back(message + " at line " + std::to_string(peek().location.line));
}

bool Parser::isVariableDeclared(const std::string& name) const {
    return declaredVariables.find(name) != declaredVariables.end();
}

void Parser::declareVariable(const std::string& name) {
    declaredVariables.insert(name);
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    
    while (!isAtEnd() && !peek().is(TokenType::END_OF_FILE)) {
        if (peek().is(TokenType::KEYWORD_IMPORT)) {
            auto imp = parseImportStatement();
            if (imp) {
                program->addImport(std::move(imp));
            } else {
                break;
            }
        } else if (peek().is(TokenType::PUNCTUATOR_HASH)) {
            auto mod = parseModule();
            if (mod) {
                program->addModule(std::move(mod));
            } else {
                break;
            }
        } else {
            addError("Expected import or module definition");
            break;
        }
    }
    
    return program;
}

std::unique_ptr<Module> Parser::parseModule() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::PUNCTUATOR_HASH)) {
        return nullptr;
    }
    
    if (!isKeywordLikeNameToken(peek().type)) {
        addError("Expected module name after '#'");
        return nullptr;
    }

    advance();
    
    std::string name = peek(-1).text;
    
    if (!consume(TokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after module name");
        return nullptr;
    }
    
    std::vector<std::unique_ptr<Function>> functions;
    
    while (!isAtEnd() && !peek().is(TokenType::PUNCTUATOR_RBRACE)) {
        auto func = parseFunction();
        if (func) {
            functions.push_back(std::move(func));
        } else {
            addError("Failed to parse function in module '" + name + "' at token: " + peek().toString());
            return nullptr;
        }
    }
    
    if (!consume(TokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close module");
        return nullptr;
    }
    
    return std::make_unique<Module>(name, std::move(functions), loc);
}

std::unique_ptr<WindowStatement> Parser::parseWindowStatement() {
    SourceLocation loc = peek().location;

    if (!consume(TokenType::KEYWORD_WINDOW)) {
        return nullptr;
    }

    if (!consume(TokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after 'window'");
        return nullptr;
    }

    auto windowDecl = std::make_unique<WindowStatement>(loc);

    while (!isAtEnd() && !peek().is(TokenType::PUNCTUATOR_RBRACE)) {
        if (peek().is(TokenType::IDENTIFIER) && peek().text == "button" && peek(1).is(TokenType::PUNCTUATOR_LBRACE)) {
            auto button = parseButtonStatement();
            if (!button) {
                return nullptr;
            }
            windowDecl->buttons.push_back(std::move(button));
            continue;
        }

        if (!consume(TokenType::IDENTIFIER)) {
            addError("Expected window property name or button block");
            return nullptr;
        }

        std::string propertyName = peek(-1).text;
        if (!consume(TokenType::PUNCTUATOR_COLON)) {
            addError("Expected ':' after window property '" + propertyName + "'");
            return nullptr;
        }

        if (propertyName == "width") {
            if (!consume(TokenType::NUMBER_LITERAL)) {
                addError("Expected integer literal for window width");
                return nullptr;
            }
            windowDecl->width = std::stoi(peek(-1).text);
        } else if (propertyName == "height" || propertyName == "high") {
            if (!consume(TokenType::NUMBER_LITERAL)) {
                addError("Expected integer literal for window height");
                return nullptr;
            }
            windowDecl->height = std::stoi(peek(-1).text);
        } else if (propertyName == "title") {
            if (consume(TokenType::STRING_LITERAL)) {
                windowDecl->title = peek(-1).text;
            } else if (consume(TokenType::IDENTIFIER)) {
                windowDecl->title = peek(-1).text;
            } else {
                addError("Expected string or identifier for window title");
                return nullptr;
            }
        } else if (propertyName == "color") {
            if (consume(TokenType::IDENTIFIER) || consume(TokenType::STRING_LITERAL)) {
                windowDecl->color = peek(-1).text;
            } else {
                addError("Expected color name for window color");
                return nullptr;
            }
        } else {
            addError("Unknown window property: " + propertyName);
            return nullptr;
        }
    }

    if (!consume(TokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close window block");
        return nullptr;
    }

    if (windowDecl->width <= 0 || windowDecl->height <= 0) {
        addError("Window width and height must be greater than zero");
        return nullptr;
    }

    return windowDecl;
}

std::unique_ptr<ButtonStatement> Parser::parseButtonStatement() {
    SourceLocation loc = peek().location;

    if (!consume(TokenType::IDENTIFIER) || peek(-1).text != "button") {
        return nullptr;
    }

    if (!consume(TokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after 'button'");
        return nullptr;
    }

    auto button = std::make_unique<ButtonStatement>(loc);

    while (!isAtEnd() && !peek().is(TokenType::PUNCTUATOR_RBRACE)) {
        if (peek().is(TokenType::IDENTIFIER) && peek(1).is(TokenType::PUNCTUATOR_COLON)) {
            advance();
            std::string propertyName = peek(-1).text;
            consume(TokenType::PUNCTUATOR_COLON);

            if (propertyName == "x") {
                if (!consume(TokenType::NUMBER_LITERAL)) {
                    addError("Expected integer literal for button x");
                    return nullptr;
                }
                button->x = std::stoi(peek(-1).text);
            } else if (propertyName == "y") {
                if (!consume(TokenType::NUMBER_LITERAL)) {
                    addError("Expected integer literal for button y");
                    return nullptr;
                }
                button->y = std::stoi(peek(-1).text);
            } else if (propertyName == "width") {
                if (!consume(TokenType::NUMBER_LITERAL)) {
                    addError("Expected integer literal for button width");
                    return nullptr;
                }
                button->width = std::stoi(peek(-1).text);
            } else if (propertyName == "height" || propertyName == "high") {
                if (!consume(TokenType::NUMBER_LITERAL)) {
                    addError("Expected integer literal for button height");
                    return nullptr;
                }
                button->height = std::stoi(peek(-1).text);
            } else if (propertyName == "text" || propertyName == "title") {
                if (consume(TokenType::STRING_LITERAL) || consume(TokenType::IDENTIFIER)) {
                    button->text = peek(-1).text;
                } else {
                    addError("Expected string or identifier for button text");
                    return nullptr;
                }
            } else {
                addError("Unknown button property: " + propertyName);
                return nullptr;
            }
            continue;
        }

        auto stmt = parseStatement();
        if (!stmt) {
            addError("Expected button property or statement");
            return nullptr;
        }
        button->body.push_back(std::move(stmt));
    }

    if (!consume(TokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close button block");
        return nullptr;
    }

    if (button->width <= 0 || button->height <= 0) {
        addError("Button width and height must be greater than zero");
        return nullptr;
    }

    return button;
}

std::unique_ptr<Function> Parser::parseFunction() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_FN)) {
        return nullptr;
    }
    
    std::string ns;
    std::string name;
    
    if (!consume(TokenType::IDENTIFIER)) {
        addError("Expected function name after 'fn'");
        return nullptr;
    }
    
    name = peek(-1).text;
    
    if (consume(TokenType::PUNCTUATOR_COLON)) {
        ns = name;
        if (!consume(TokenType::IDENTIFIER)) {
            addError("Expected function name after namespace ':'");
            return nullptr;
        }
        name = peek(-1).text;
    }
    
    if (!consume(TokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after function name");
        return nullptr;
    }
    
    std::vector<std::unique_ptr<VariableDeclaration>> params;
    
    if (!peek().is(TokenType::PUNCTUATOR_RPAREN)) {
        while (true) {
            if (!consume(TokenType::IDENTIFIER)) {
                addError("Expected parameter name");
                return nullptr;
            }
            std::string param_name = peek(-1).text;
            params.push_back(std::make_unique<VariableDeclaration>(param_name, loc));
            
            if (consume(TokenType::PUNCTUATOR_COMMA)) {
                continue;
            }
            break;
        }
    }
    
    if (!consume(TokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after function parameters");
        return nullptr;
    }
    
    auto body = parseBlockStatement();
    if (!body) {
        return nullptr;
    }
    
    if (ns.empty()) {
        return std::make_unique<Function>(name, std::move(params), std::move(body), loc);
    } else {
        return std::make_unique<Function>(name, ns, std::move(params), std::move(body), loc);
    }
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) return nullptr;
    
    if (peek().is(TokenType::KEYWORD_IMPORT)) {
        return parseImportStatement();
    } else if (peek().is(TokenType::KEYWORD_PRINT)) {
        return parsePrintStatement();
    } else if (peek().is(TokenType::KEYWORD_WHILE)) {
        return parseWhileStatement();
    } else if (peek().is(TokenType::KEYWORD_IF)) {
        return parseIfStatement();
    } else if (peek().is(TokenType::KEYWORD_BREAK)) {
        return parseBreakStatement();
    } else if (peek().is(TokenType::KEYWORD_RETURN)) {
        return parseReturnStatement();
    } else if (peek().is(TokenType::KEYWORD_FOR)) {
        return parseForInStatement();
    } else if (peek().is(TokenType::KEYWORD_WINDOW)) {
        return parseWindowStatement();
    } else if (peek().is(TokenType::KEYWORD_FN)) {
        SourceLocation loc = peek().location;
        auto func = parseFunction();
        if (!func) return nullptr;
        return std::make_unique<FunctionDeclarationStatement>(std::move(func), loc);
    } else if (peek().is(TokenType::KEYWORD_INT)) {
        advance();
        return parseTypedAssignmentStatement(VarType::INT);
    } else if (peek().is(TokenType::KEYWORD_FLOAT)) {
        advance();
        return parseTypedAssignmentStatement(VarType::FLOAT);
    } else if (peek().is(TokenType::KEYWORD_BOOL)) {
        advance();
        return parseTypedAssignmentStatement(VarType::BOOL);
    } else if (peek().is(TokenType::PUNCTUATOR_LBRACE)) {
        return parseBlockStatement();
    } else if (peek().is(TokenType::IDENTIFIER)) {
        if (peek(1).is(TokenType::PUNCTUATOR_LPAREN) || peek(1).is(TokenType::PUNCTUATOR_COLON)) {
            auto expr = parseExpression();
            if (!expr) return nullptr;
            return std::make_unique<ExpressionStatement>(std::move(expr), peek().location);
        } else {
            return parseAssignmentStatement();
        }
    } else {
        addError("Unexpected token: " + peek().toString());
        advance();
        return nullptr;
    }
}

std::unique_ptr<PrintStatement> Parser::parsePrintStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_PRINT)) {
        return nullptr;
    }
    
    if (!consume(TokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after 'print'");
        return nullptr;
    }
    
    auto expr = parseExpression();
    if (!expr) {
        return nullptr;
    }
    
    if (!consume(TokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after print argument");
        return nullptr;
    }
    
    return std::make_unique<PrintStatement>(std::move(expr), loc);
}

std::unique_ptr<AssignmentStatement> Parser::parseAssignmentStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::IDENTIFIER)) {
        return nullptr;
    }
    
    std::string name = peek(-1).text;
    
    if (!consume(TokenType::PUNCTUATOR_EQUAL)) {
        addError("Expected '=' after variable name");
        return nullptr;
    }
    
    auto expr = parseExpression();
    if (!expr) {
        return nullptr;
    }
    
    bool alreadyDeclared = isVariableDeclared(name);
    if (!alreadyDeclared) {
        addWarning("Implicit type declaration for variable '" + name + "'. Consider using explicit type declaration (e.g., int " + name + " = ...)");
        declareVariable(name);
    }
    
    auto stmt = std::make_unique<AssignmentStatement>(name, std::move(expr), loc);
    stmt->isReassignment = alreadyDeclared;
    return stmt;
}

std::unique_ptr<AssignmentStatement> Parser::parseTypedAssignmentStatement(VarType type) {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::IDENTIFIER)) {
        addError("Expected variable name after type");
        return nullptr;
    }
    
    std::string name = peek(-1).text;
    
    if (!consume(TokenType::PUNCTUATOR_EQUAL)) {
        addError("Expected '=' after variable name");
        return nullptr;
    }
    
    auto expr = parseExpression();
    if (!expr) {
        return nullptr;
    }
    
    declareVariable(name);
    
    return std::make_unique<AssignmentStatement>(name, std::move(expr), type, loc);
}

std::unique_ptr<BreakStatement> Parser::parseBreakStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_BREAK)) {
        return nullptr;
    }
    
    return std::make_unique<BreakStatement>(loc);
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_RETURN)) {
        return nullptr;
    }
    
    auto expr = parseExpression();
    if (!expr) {
        return nullptr;
    }
    
    return std::make_unique<ReturnStatement>(std::move(expr), loc);
}

std::unique_ptr<BlockStatement> Parser::parseBlockStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::PUNCTUATOR_LBRACE)) {
        return nullptr;
    }
    
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd() && !peek().is(TokenType::PUNCTUATOR_RBRACE)) {
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        } else {
            return nullptr;
        }
    }
    
    if (!consume(TokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close block");
        return nullptr;
    }
    
    return std::make_unique<BlockStatement>(std::move(statements), loc);
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_WHILE)) {
        return nullptr;
    }
    
    if (!consume(TokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after 'while'");
        return nullptr;
    }
    
    auto cond = parseExpression();
    if (!cond) {
        return nullptr;
    }
    
    if (!consume(TokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after while condition");
        return nullptr;
    }
    
    auto body = parseStatement();
    if (!body) {
        return nullptr;
    }
    
    return std::make_unique<WhileStatement>(std::move(cond), std::move(body), loc);
}

std::unique_ptr<ForInStatement> Parser::parseForInStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_FOR)) {
        return nullptr;
    }
    
    if (!consume(TokenType::IDENTIFIER)) {
        addError("Expected variable name after 'for'");
        return nullptr;
    }
    
    std::string varName = peek(-1).text;
    
    if (!consume(TokenType::IDENTIFIER) || peek(-1).text != "in") {
        addError("Expected 'in' after loop variable");
        return nullptr;
    }
    
    auto iterable = parseExpression();
    if (!iterable) {
        return nullptr;
    }
    
    auto body = parseStatement();
    if (!body) {
        return nullptr;
    }
    
    return std::make_unique<ForInStatement>(varName, std::move(iterable), std::move(body), loc);
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_IF)) {
        return nullptr;
    }
    
    auto cond = parseExpression();
    if (!cond) {
        return nullptr;
    }
    
    auto thenBranch = parseStatement();
    if (!thenBranch) {
        return nullptr;
    }
    
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> elseIfBranches;
    while (true) {
        size_t savedPos = current;
        if (consume(TokenType::KEYWORD_ELSE) && consume(TokenType::KEYWORD_IF)) {
            auto elseIfCond = parseExpression();
            if (!elseIfCond) {
                return nullptr;
            }
            auto elseIfThen = parseStatement();
            if (!elseIfThen) {
                return nullptr;
            }
            elseIfBranches.push_back({std::move(elseIfCond), std::move(elseIfThen)});
        } else {
            current = savedPos;
            break;
        }
    }
    
    std::unique_ptr<Statement> elseBranch = nullptr;
    if (consume(TokenType::KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }
    
    auto ifStmt = std::make_unique<IfStatement>(std::move(cond), std::move(thenBranch), std::move(elseBranch), loc);
    ifStmt->elseIfBranches = std::move(elseIfBranches);
    
    return ifStmt;
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    
    while (!isAtEnd() && peek().is(TokenType::PUNCTUATOR_OR)) {
        BinaryOpType op = BinaryOpType::OR;
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto left = parseEquality();
    
    while (!isAtEnd() && peek().is(TokenType::PUNCTUATOR_AND)) {
        BinaryOpType op = BinaryOpType::AND;
        advance();
        auto right = parseEquality();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto left = parseRelational();
    
    while (!isAtEnd()) {
        BinaryOpType op;
        if (consume(TokenType::PUNCTUATOR_EQUAL_EQUAL)) {
            op = BinaryOpType::EQUAL;
        } else if (consume(TokenType::PUNCTUATOR_EXCLAIM_EQUAL)) {
            op = BinaryOpType::NOT_EQUAL;
        } else {
            break;
        }
        auto right = parseRelational();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseRelational() {
    auto left = parseAdditive();
    
    while (!isAtEnd()) {
        BinaryOpType op;
        if (consume(TokenType::PUNCTUATOR_LESS_EQUAL)) {
            op = BinaryOpType::LESS_EQUAL;
        } else if (consume(TokenType::PUNCTUATOR_GREATER_EQUAL)) {
            op = BinaryOpType::GREATER_EQUAL;
        } else if (consume(TokenType::PUNCTUATOR_LESS)) {
            op = BinaryOpType::LESS;
        } else if (consume(TokenType::PUNCTUATOR_GREATER)) {
            op = BinaryOpType::GREATER;
        } else {
            break;
        }
        auto right = parseAdditive();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    
    while (!isAtEnd()) {
        BinaryOpType op;
        if (consume(TokenType::PUNCTUATOR_PLUS)) {
            op = BinaryOpType::ADD;
        } else if (consume(TokenType::PUNCTUATOR_MINUS)) {
            op = BinaryOpType::SUB;
        } else {
            break;
        }
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseMultiplicative() {
    auto left = parseUnary();
    
    while (!isAtEnd()) {
        BinaryOpType op;
        if (consume(TokenType::PUNCTUATOR_STAR)) {
            op = BinaryOpType::MUL;
        } else if (consume(TokenType::PUNCTUATOR_SLASH)) {
            op = BinaryOpType::DIV;
        } else if (consume(TokenType::PUNCTUATOR_PERCENT)) {
            op = BinaryOpType::MOD;
        } else {
            break;
        }
        auto right = parseUnary();
        left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right), peek().location);
    }
    
    return left;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (consume(TokenType::PUNCTUATOR_MINUS)) {
        auto expr = parseUnary();
        return std::make_unique<UnaryOp>(UnaryOpType::NEGATE, std::move(expr), peek().location);
    }
    
    if (consume(TokenType::PUNCTUATOR_EXCLAIM)) {
        auto expr = parseUnary();
        return std::make_unique<UnaryOp>(UnaryOpType::NOT, std::move(expr), peek().location);
    }
    
    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (consume(TokenType::NUMBER_LITERAL)) {
        int64_t value = std::stoll(peek(-1).text);
        return std::make_unique<NumberLiteral>(value, peek(-1).location);
    }
    
    if (consume(TokenType::FLOAT_LITERAL)) {
        double value = std::stod(peek(-1).text);
        return std::make_unique<FloatLiteral>(value, peek(-1).location);
    }
    
    if (consume(TokenType::KEYWORD_TRUE)) {
        return std::make_unique<BooleanLiteral>(true, peek(-1).location);
    }
    
    if (consume(TokenType::KEYWORD_FALSE)) {
        return std::make_unique<BooleanLiteral>(false, peek(-1).location);
    }
    
    if (consume(TokenType::STRING_LITERAL)) {
        std::string value = peek(-1).text;
        return std::make_unique<StringLiteral>(value, peek(-1).location);
    }
    
    if (peek().is(TokenType::PUNCTUATOR_LBRACKET)) {
        return parseArrayLiteral();
    }
    
    if (consume(TokenType::PUNCTUATOR_LPAREN)) {
        auto expr = parseExpression();
        if (!consume(TokenType::PUNCTUATOR_RPAREN)) {
            addError("Expected ')' after expression");
            return nullptr;
        }
        return expr;
    }
    
    if (consume(TokenType::IDENTIFIER)) {
        std::string name = peek(-1).text;
        SourceLocation loc = peek(-1).location;
        
        std::string ns;
        if (consume(TokenType::PUNCTUATOR_COLON)) {
            ns = name;
            if (!consume(TokenType::IDENTIFIER)) {
                addError("Expected function name after namespace ':'");
                return nullptr;
            }
            name = peek(-1).text;
        }
        
        if (consume(TokenType::PUNCTUATOR_LPAREN)) {
            std::vector<std::unique_ptr<Expression>> args;
            
            if (!peek().is(TokenType::PUNCTUATOR_RPAREN)) {
                while (true) {
                    auto arg = parseExpression();
                    if (arg) {
                        args.push_back(std::move(arg));
                    } else {
                        return nullptr;
                    }
                    
                    if (consume(TokenType::PUNCTUATOR_COMMA)) {
                        continue;
                    }
                    break;
                }
            }
            
            if (!consume(TokenType::PUNCTUATOR_RPAREN)) {
                addError("Expected ')' after function arguments");
                return nullptr;
            }
            
            std::unique_ptr<Expression> result;
            if (ns.empty()) {
                result = std::make_unique<CallExpression>(name, std::move(args), loc);
            } else {
                result = std::make_unique<CallExpression>(name, ns, std::move(args), loc);
            }
            return parsePostfix(std::move(result));
        }
        
        if (consume(TokenType::PUNCTUATOR_LBRACKET)) {
            auto startExpr = parseExpression();
            if (!startExpr) {
                addError("Expected expression for array range start");
                return nullptr;
            }
            
            if (!consume(TokenType::PUNCTUATOR_DOT_DOT_DOT)) {
                if (!consume(TokenType::PUNCTUATOR_RBRACKET)) {
                    addError("Expected ']' after array index");
                    return nullptr;
                }
                auto varExpr = std::make_unique<VariableExpression>(name, loc);
                return std::make_unique<ArrayIndexExpression>(std::move(varExpr), std::move(startExpr), loc);
            }
            
            auto endExpr = parseExpression();
            if (!endExpr) {
                addError("Expected expression for array range end");
                return nullptr;
            }
            
            if (!consume(TokenType::PUNCTUATOR_RBRACKET)) {
                addError("Expected ']' after array range");
                return nullptr;
            }
            
            return std::make_unique<ArrayRangeExpression>(name, std::move(startExpr), std::move(endExpr), loc);
        }
        
        auto varExpr = std::make_unique<VariableExpression>(name, loc);
        return parsePostfix(std::move(varExpr));
    }
    
    addError("Expected expression, got: " + peek().toString());
    return nullptr;
}

std::unique_ptr<Expression> Parser::parseArrayLiteral() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::PUNCTUATOR_LBRACKET)) {
        return nullptr;
    }
    
    if (peek().is(TokenType::PUNCTUATOR_RBRACKET)) {
        advance();
        return std::make_unique<ArrayLiteral>(std::vector<std::unique_ptr<Expression>>(), loc);
    }
    
    auto firstExpr = parseExpression();
    if (!firstExpr) {
        return nullptr;
    }
    
    if (consume(TokenType::PUNCTUATOR_DOT_DOT_DOT)) {
        auto endExpr = parseExpression();
        if (!endExpr) {
            addError("Expected expression after '...' in range");
            return nullptr;
        }
        
        if (!consume(TokenType::PUNCTUATOR_RBRACKET)) {
            addError("Expected ']' after range");
            return nullptr;
        }
        
        return std::make_unique<ArrayLiteral>(std::move(firstExpr), std::move(endExpr), loc);
    }
    
    std::vector<std::unique_ptr<Expression>> elements;
    elements.push_back(std::move(firstExpr));
    
    while (consume(TokenType::PUNCTUATOR_COMMA)) {
        auto elem = parseExpression();
        if (!elem) {
            return nullptr;
        }
        elements.push_back(std::move(elem));
    }
    
    if (!consume(TokenType::PUNCTUATOR_RBRACKET)) {
        addError("Expected ']' after array elements");
        return nullptr;
    }
    
    return std::make_unique<ArrayLiteral>(std::move(elements), loc);
}

std::unique_ptr<Expression> Parser::parsePostfix(std::unique_ptr<Expression> expr) {
    while (peek().is(TokenType::PUNCTUATOR_LBRACKET)) {
        advance();
        auto indexExpr = parseExpression();
        if (!indexExpr) {
            return nullptr;
        }
        
        if (!consume(TokenType::PUNCTUATOR_RBRACKET)) {
            addError("Expected ']' after array index");
            return nullptr;
        }
        
        expr = std::make_unique<ArrayIndexExpression>(std::move(expr), std::move(indexExpr), expr->location);
    }
    
    return expr;
}

std::unique_ptr<ImportStatement> Parser::parseImportStatement() {
    SourceLocation loc = peek().location;
    
    if (!consume(TokenType::KEYWORD_IMPORT)) {
        return nullptr;
    }
    
    if (!consume(TokenType::STRING_LITERAL)) {
        addError("Expected string literal after 'import'");
        return nullptr;
    }
    
    std::string name = peek(-1).text;
    
    std::string path;
    ImportType type = ImportType::MOD;
    
    size_t lastDot = name.find_last_of('.');
    if (lastDot != std::string::npos && lastDot < name.length() - 1) {
        std::string ext = name.substr(lastDot);
        if (ext == ".xfmod") {
            type = ImportType::MOD;
            path = "mods/" + name;
        } else {
            type = ImportType::FILE;
            path = name;
        }
    } else {
        path = "mods/" + name + ".xfmod";
    }
    
    return std::make_unique<ImportStatement>(name, path, type, loc);
}

std::string Parser::statementTypeToString(Statement* stmt) {
    if (dynamic_cast<PrintStatement*>(stmt)) return "print";
    if (dynamic_cast<AssignmentStatement*>(stmt)) return "assignment";
    if (dynamic_cast<WhileStatement*>(stmt)) return "while";
    if (dynamic_cast<IfStatement*>(stmt)) return "if";
    if (dynamic_cast<BreakStatement*>(stmt)) return "break";
    if (dynamic_cast<ReturnStatement*>(stmt)) return "return";
    if (dynamic_cast<BlockStatement*>(stmt)) return "block";
    if (dynamic_cast<ImportStatement*>(stmt)) return "import";
    return "unknown";
}

}
