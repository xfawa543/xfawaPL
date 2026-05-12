#ifndef XFAWA_AST_H
#define XFAWA_AST_H

#include "xfawa_types.h"
#include <memory>
#include <vector>
#include <string>

namespace xfawa {

class StringLiteral : public Expression {
public:
    std::string value;
    
    StringLiteral(const std::string& val, const SourceLocation& loc = SourceLocation()) 
        : Expression(NodeType::STRING_LITERAL, loc), value(val) {}
    
    std::string toString() const override {
        return "\"" + value + "\"";
    }
};

class NumberLiteral : public Expression {
public:
    int64_t value;
    
    NumberLiteral(int64_t val, const SourceLocation& loc = SourceLocation()) 
        : Expression(NodeType::NUMBER_LITERAL, loc), value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

class FloatLiteral : public Expression {
public:
    double value;
    
    FloatLiteral(double val, const SourceLocation& loc = SourceLocation()) 
        : Expression(NodeType::FLOAT_LITERAL, loc), value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
};

class BooleanLiteral : public Expression {
public:
    bool value;
    
    
    BooleanLiteral(bool val, const SourceLocation& loc = SourceLocation()) 
        : Expression(NodeType::BOOLEAN_LITERAL, loc), value(val) {}
    
    std::string toString() const override {
        return value ? "true" : "false";
    }
};

class VariableExpression : public Expression {
public:
    std::string name;
    
    VariableExpression(const std::string& n, const SourceLocation& loc = SourceLocation()) 
        : Expression(NodeType::VARIABLE_EXPRESSION, loc), name(n) {}
    
    std::string toString() const override {
        return name;
    }
};

class BinaryOp : public Expression {
public:
    BinaryOpType op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    
    BinaryOp(BinaryOpType o, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r,
             const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::BINARY_OP, loc), op(o), left(std::move(l)), right(std::move(r)) {}
    
    std::string toString() const override {
        std::string result = left->toString();
        switch (op) {
            case BinaryOpType::ADD: result += " + "; break;
            case BinaryOpType::SUB: result += " - "; break;
            case BinaryOpType::MUL: result += " * "; break;
            case BinaryOpType::DIV: result += " / "; break;
            case BinaryOpType::MOD: result += " % "; break;
            case BinaryOpType::EQUAL: result += " == "; break;
            case BinaryOpType::NOT_EQUAL: result += " != "; break;
            case BinaryOpType::LESS: result += " < "; break;
            case BinaryOpType::LESS_EQUAL: result += " <= "; break;
            case BinaryOpType::GREATER: result += " > "; break;
            case BinaryOpType::GREATER_EQUAL: result += " >= "; break;
            case BinaryOpType::AND: result += " && "; break;
            case BinaryOpType::OR: result += " || "; break;
            default: result += " ? "; break;
        }
        result += right->toString();
        return result;
    }
};

class UnaryOp : public Expression {
public:
    UnaryOpType op;
    std::unique_ptr<Expression> expr;
    
    UnaryOp(UnaryOpType o, std::unique_ptr<Expression> e,
            const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::UNARY_OP, loc), op(o), expr(std::move(e)) {}
    
    std::string toString() const override {
        std::string result;
        switch (op) {
            case UnaryOpType::NEGATE: result = "-"; break;
            case UnaryOpType::NOT: result = "!"; break;
            default: result = "?"; break;
        }
        result += "(" + expr->toString() + ")";
        return result;
    }
};

class CallExpression : public Expression {
public:
    std::string name;
    std::string ns;
    std::vector<std::unique_ptr<Expression>> args;
    
    CallExpression(const std::string& n, std::vector<std::unique_ptr<Expression>> a,
                   const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::CALL_EXPRESSION, loc), name(n), ns(""), args(std::move(a)) {}
    
    CallExpression(const std::string& n, const std::string& ns_name, std::vector<std::unique_ptr<Expression>> a,
                   const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::CALL_EXPRESSION, loc), name(n), ns(ns_name), args(std::move(a)) {}
    
    std::string toString() const override {
        std::string result;
        if (!ns.empty()) {
            result = ns + ":" + name + "(";
        } else {
            result = name + "(";
        }
        for (size_t i = 0; i < args.size(); i++) {
            result += args[i]->toString();
            if (i < args.size() - 1) result += ", ";
        }
        result += ")";
        return result;
    }
};

enum class ArrayAccessType {
    SEQUENTIAL,
    RANDOM,
    RECIPROCAL
};

class ArrayRangeExpression : public Expression {
public:
    std::string accessType;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::unique_ptr<Expression> array;
    bool isSlice;
    
    ArrayRangeExpression(const std::string& access, std::unique_ptr<Expression> s, std::unique_ptr<Expression> e,
                          const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::ARRAY_RANGE_EXPRESSION, loc), accessType(access), start(std::move(s)), end(std::move(e)), isSlice(false) {}
    
    ArrayRangeExpression(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> s, std::unique_ptr<Expression> e,
                          const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::ARRAY_RANGE_EXPRESSION, loc), start(std::move(s)), end(std::move(e)), array(std::move(arr)), isSlice(true) {}
    
    std::string toString() const override {
        if (isSlice && array) {
            return array->toString() + "[" + start->toString() + "..." + end->toString() + "]";
        }
        return accessType + "[" + start->toString() + "..." + end->toString() + "]";
    }
};

class ArrayLiteral : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    bool isRange;
    std::unique_ptr<Expression> rangeStart;
    std::unique_ptr<Expression> rangeEnd;
    
    ArrayLiteral(std::vector<std::unique_ptr<Expression>> elems, const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::ARRAY_LITERAL, loc), elements(std::move(elems)), isRange(false) {}
    
    ArrayLiteral(std::unique_ptr<Expression> start, std::unique_ptr<Expression> end, const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::ARRAY_LITERAL, loc), isRange(true), rangeStart(std::move(start)), rangeEnd(std::move(end)) {}
    
    std::string toString() const override {
        if (isRange) {
            return "[" + rangeStart->toString() + "..." + rangeEnd->toString() + "]";
        }
        std::string result = "[";
        for (size_t i = 0; i < elements.size(); i++) {
            result += elements[i]->toString();
            if (i < elements.size() - 1) result += ", ";
        }
        result += "]";
        return result;
    }
};

class ArrayIndexExpression : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
    
    ArrayIndexExpression(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx,
                          const SourceLocation& loc = SourceLocation())
        : Expression(NodeType::ARRAY_INDEX_EXPRESSION, loc), array(std::move(arr)), index(std::move(idx)) {}
    
    std::string toString() const override {
        return array->toString() + "[" + index->toString() + "]";
    }
};

class VariableDeclaration {
public:
    std::string name;
    SourceLocation location;
    
    VariableDeclaration(const std::string& n, const SourceLocation& loc = SourceLocation()) 
        : name(n), location(loc) {}
};

class PrintStatement : public Statement {
public:
    std::unique_ptr<Expression> expr;
    std::string outputTarget;
    
    PrintStatement(std::unique_ptr<Expression> e, const SourceLocation& loc = SourceLocation()) 
        : Statement(NodeType::PRINT_STATEMENT, loc), expr(std::move(e)) {}
    
    std::string toString() const override {
        if (!outputTarget.empty()) {
            return "print(" + expr->toString() + ", " + outputTarget + ")";
        }
        return "print(" + expr->toString() + ")";
    }
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expr;
    
    ExpressionStatement(std::unique_ptr<Expression> e, const SourceLocation& loc = SourceLocation()) 
        : Statement(NodeType::EXPRESSION_STATEMENT, loc), expr(std::move(e)) {}
    
    std::string toString() const override {
        return expr->toString();
    }
};

class AssignmentStatement : public Statement {
public:
    std::string name;
    std::unique_ptr<Expression> value;
    VarType declaredType;
    bool hasExplicitType;
    bool isReassignment;
    
    AssignmentStatement(const std::string& n, std::unique_ptr<Expression> v,
                       const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::ASSIGNMENT_STATEMENT, loc), name(n), value(std::move(v)), 
          declaredType(VarType::UNKNOWN), hasExplicitType(false), isReassignment(false) {}
    
    AssignmentStatement(const std::string& n, std::unique_ptr<Expression> v, VarType t,
                       const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::ASSIGNMENT_STATEMENT, loc), name(n), value(std::move(v)), 
          declaredType(t), hasExplicitType(true), isReassignment(false) {}
    
    std::string toString() const override {
        if (hasExplicitType) {
            return varTypeToString(declaredType) + " " + name + " = " + value->toString();
        }
        return name + " = " + value->toString();
    }
};

class BreakStatement : public Statement {
public:
    BreakStatement(const SourceLocation& loc = SourceLocation()) 
        : Statement(NodeType::BREAK_STATEMENT, loc) {}
    
    std::string toString() const override {
        return "break";
    }
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value;
    
    ReturnStatement(std::unique_ptr<Expression> v, const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::RETURN_STATEMENT, loc), value(std::move(v)) {}
    
    std::string toString() const override {
        return "return " + value->toString();
    }
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    BlockStatement(const SourceLocation& loc = SourceLocation()) 
        : Statement(NodeType::BLOCK_STATEMENT, loc) {}
    BlockStatement(std::vector<std::unique_ptr<Statement>> stmts, const SourceLocation& loc = SourceLocation()) 
        : Statement(NodeType::BLOCK_STATEMENT, loc), statements(std::move(stmts)) {}
    
    void addStatement(std::unique_ptr<Statement> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    std::string toString() const override {
        std::string result = "{\n";
        for (const auto& stmt : statements) {
            result += "  " + stmt->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> b,
                   const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::WHILE_STATEMENT, loc), condition(std::move(c)), body(std::move(b)) {}
    
    std::string toString() const override {
        return "while (" + condition->toString() + ") " + body->toString();
    }
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> elseIfBranches;
    
    IfStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> t,
                std::unique_ptr<Statement> e,
                const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::IF_STATEMENT, loc), condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    
    std::string toString() const override {
        std::string result = "if " + condition->toString() + " " + thenBranch->toString();
        for (const auto& elseIf : elseIfBranches) {
            result += " else if " + elseIf.first->toString() + " " + elseIf.second->toString();
        }
        if (elseBranch) {
            result += " else " + elseBranch->toString();
        }
        return result;
    }
};

class ForInStatement : public Statement {
public:
    std::string varName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    
    ForInStatement(const std::string& var, std::unique_ptr<Expression> iter, std::unique_ptr<Statement> b,
                   const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::FOR_IN_STATEMENT, loc), varName(var), iterable(std::move(iter)), body(std::move(b)) {}
    
    std::string toString() const override {
        return "for " + varName + " in " + iterable->toString() + " " + body->toString();
    }
};

class Function {
public:
    std::string name;
    std::string ns;
    std::vector<std::unique_ptr<VariableDeclaration>> params;
    std::unique_ptr<BlockStatement> body;
    SourceLocation location;
    
    Function(const std::string& n, std::vector<std::unique_ptr<VariableDeclaration>> p,
             std::unique_ptr<BlockStatement> b, const SourceLocation& loc = SourceLocation())
        : name(n), ns(""), params(std::move(p)), body(std::move(b)), location(loc) {}
    
    Function(const std::string& n, const std::string& ns_name, std::vector<std::unique_ptr<VariableDeclaration>> p,
             std::unique_ptr<BlockStatement> b, const SourceLocation& loc = SourceLocation())
        : name(n), ns(ns_name), params(std::move(p)), body(std::move(b)), location(loc) {}
    
    std::string toString() const {
        std::string result = "fn ";
        if (!ns.empty()) {
            result += ns + ":";
        }
        result += name + "(";
        for (size_t i = 0; i < params.size(); i++) {
            result += params[i]->name;
            if (i < params.size() - 1) result += ", ";
        }
        result += ") " + body->toString();
        return result;
    }
};

class Module {
public:
    std::string name;
    std::vector<std::unique_ptr<Function>> functions;
    SourceLocation location;
    
    Module(const std::string& n, std::vector<std::unique_ptr<Function>> f,
           const SourceLocation& loc = SourceLocation())
        : name(n), functions(std::move(f)), location(loc) {}
    
    std::string toString() const {
        std::string result = "#" + name + " {\n";
        for (const auto& func : functions) {
            result += "  " + func->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class ButtonStatement : public Statement {
public:
    int x = 20;
    int y = 20;
    int width = 120;
    int height = 36;
    std::string text = "button";
    std::vector<std::unique_ptr<Statement>> body;

    ButtonStatement(const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::BUTTON_STATEMENT, loc) {}

    std::string toString() const override {
        std::string result = "button {\n"
                             "    x: " + std::to_string(x) + "\n"
                             "    y: " + std::to_string(y) + "\n"
                             "    width: " + std::to_string(width) + "\n"
                             "    height: " + std::to_string(height) + "\n"
                             "    text: \"" + text + "\"\n";
        for (const auto& stmt : body) {
            result += "    " + stmt->toString() + "\n";
        }
        result += "  }";
        return result;
    }
};

class TextStatement : public Statement {
public:
    int x = 20;
    int y = 20;
    int width = 160;
    int height = 24;
    std::string text = "text";

    TextStatement(const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::TEXT_STATEMENT, loc) {}

    std::string toString() const override {
        return "text {\n"
               "    x: " + std::to_string(x) + "\n"
               "    y: " + std::to_string(y) + "\n"
               "    width: " + std::to_string(width) + "\n"
               "    height: " + std::to_string(height) + "\n"
               "    text: \"" + text + "\"\n"
               "  }";
    }
};

class BoxStatement : public Statement {
public:
    int x = 20;
    int y = 20;
    int width = 240;
    int height = 120;
    std::string id = "output";
    std::string text = "";

    BoxStatement(const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::BOX_STATEMENT, loc) {}

    std::string toString() const override {
        return "box {\n"
               "    id: " + id + "\n"
               "    x: " + std::to_string(x) + "\n"
               "    y: " + std::to_string(y) + "\n"
               "    width: " + std::to_string(width) + "\n"
               "    height: " + std::to_string(height) + "\n"
               "    text: \"" + text + "\"\n"
               "  }";
    }
};

class InputStatement : public Statement {
public:
    int x = 20;
    int y = 20;
    int width = 200;
    int height = 32;
    std::string id = "input";
    std::string varName = "";

    InputStatement(const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::INPUT_STATEMENT, loc) {}

    std::string toString() const override {
        return "input {\n"
               "    id: " + id + "\n"
               "    x: " + std::to_string(x) + "\n"
               "    y: " + std::to_string(y) + "\n"
               "    width: " + std::to_string(width) + "\n"
               "    height: " + std::to_string(height) + "\n"
               "    var: " + varName + "\n"
               "  }";
    }
};

class WindowStatement : public Statement {
public:
    int width = 800;
    int height = 600;
    std::string title = "xfawa";
    std::string color = "white";
    std::string style = "";
    std::vector<std::unique_ptr<ButtonStatement>> buttons;
    std::vector<std::unique_ptr<TextStatement>> texts;
    std::vector<std::unique_ptr<BoxStatement>> boxes;
    std::vector<std::unique_ptr<InputStatement>> inputs;

    WindowStatement(const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::WINDOW_STATEMENT, loc) {}

    std::string toString() const override {
        std::string result = "window {\n"
               "  width: " + std::to_string(width) + "\n"
               "  height: " + std::to_string(height) + "\n"
               "  title: \"" + title + "\"\n"
               "  color: " + color + "\n";
        if (!style.empty()) {
            result += "  style: \"" + style + "\"\n";
        }
        for (const auto& button : buttons) {
            result += "  " + button->toString() + "\n";
        }
        for (const auto& textItem : texts) {
            result += "  " + textItem->toString() + "\n";
        }
        for (const auto& box : boxes) {
            result += "  " + box->toString() + "\n";
        }
        for (const auto& input : inputs) {
            result += "  " + input->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class Program {
public:
    std::vector<std::unique_ptr<Module>> modules;
    std::vector<std::unique_ptr<ImportStatement>> imports;
    
    void addModule(std::unique_ptr<Module> mod) {
        modules.push_back(std::move(mod));
    }
    
    void addImport(std::unique_ptr<ImportStatement> imp) {
        imports.push_back(std::move(imp));
    }
    
    std::string toString() const {
        std::string result;
        for (const auto& imp : imports) {
            result += imp->toString() + "\n";
        }
        for (const auto& mod : modules) {
            result += mod->toString() + "\n";
        }
        return result;
    }
};

class FunctionDeclarationStatement : public Statement {
public:
    std::unique_ptr<Function> func;
    
    FunctionDeclarationStatement(std::unique_ptr<Function> f, const SourceLocation& loc = SourceLocation())
        : Statement(NodeType::FUNCTION_DECLARATION, loc), func(std::move(f)) {}
    
    std::string toString() const override {
        return func ? func->toString() : "fn <null>";
    }
};

}

#endif
