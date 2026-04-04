#include "xfawa_lexer.h"
#include <vector>
#include <cctype>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace xfawa {

const std::vector<std::pair<std::string, TokenType>> Lexer::keywords = {
    {"fn", TokenType::KEYWORD_FN},
    {"if", TokenType::KEYWORD_IF},
    {"else", TokenType::KEYWORD_ELSE},
    {"while", TokenType::KEYWORD_WHILE},
    {"break", TokenType::KEYWORD_BREAK},
    {"return", TokenType::KEYWORD_RETURN},
    {"true", TokenType::KEYWORD_TRUE},
    {"false", TokenType::KEYWORD_FALSE},
    {"print", TokenType::KEYWORD_PRINT},
    {"import", TokenType::KEYWORD_IMPORT},
    {"%import", TokenType::KEYWORD_PERCENT_IMPORT},
    {"int", TokenType::KEYWORD_INT},
    {"float", TokenType::KEYWORD_FLOAT},
    {"bool", TokenType::KEYWORD_BOOL},
    {"for", TokenType::KEYWORD_FOR},
    {"window", TokenType::KEYWORD_WINDOW}
};

const std::vector<std::pair<std::string, TokenType>> Lexer::punctuatuators = {
    {"(", TokenType::PUNCTUATOR_LPAREN},
    {")", TokenType::PUNCTUATOR_RPAREN},
    {"{", TokenType::PUNCTUATOR_LBRACE},
    {"}", TokenType::PUNCTUATOR_RBRACE},
    {"[", TokenType::PUNCTUATOR_LBRACKET},
    {"]", TokenType::PUNCTUATOR_RBRACKET},
    {";", TokenType::PUNCTUATOR_SEMICOLON},
    {",", TokenType::PUNCTUATOR_COMMA},
    {":", TokenType::PUNCTUATOR_COLON},
    {"+", TokenType::PUNCTUATOR_PLUS},
    {"-", TokenType::PUNCTUATOR_MINUS},
    {"*", TokenType::PUNCTUATOR_STAR},
    {"/", TokenType::PUNCTUATOR_SLASH},
    {"%", TokenType::PUNCTUATOR_PERCENT},
    {"=", TokenType::PUNCTUATOR_EQUAL},
    {"==", TokenType::PUNCTUATOR_EQUAL_EQUAL},
    {"!", TokenType::PUNCTUATOR_EXCLAIM},
    {"!=", TokenType::PUNCTUATOR_EXCLAIM_EQUAL},
    {"<", TokenType::PUNCTUATOR_LESS},
    {"<=", TokenType::PUNCTUATOR_LESS_EQUAL},
    {">", TokenType::PUNCTUATOR_GREATER},
    {">=", TokenType::PUNCTUATOR_GREATER_EQUAL},
    {"&&", TokenType::PUNCTUATOR_AND},
    {"||", TokenType::PUNCTUATOR_OR},
    {"#", TokenType::PUNCTUATOR_HASH},
    {"$", TokenType::PUNCTUATOR_DOLLAR},
    {"...", TokenType::PUNCTUATOR_DOT_DOT_DOT}
};

std::string Lexer::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::IDENTIFIER: return "identifier";
        case TokenType::NUMBER_LITERAL: return "number";
        case TokenType::STRING_LITERAL: return "string";
        case TokenType::FLOAT_LITERAL: return "float";
        case TokenType::KEYWORD_FN: return "fn";
        case TokenType::KEYWORD_IF: return "if";
        case TokenType::KEYWORD_ELSE: return "else";
        case TokenType::KEYWORD_WHILE: return "while";
        case TokenType::KEYWORD_BREAK: return "break";
        case TokenType::KEYWORD_RETURN: return "return";
        case TokenType::KEYWORD_TRUE: return "true";
        case TokenType::KEYWORD_FALSE: return "false";
        case TokenType::KEYWORD_PRINT: return "print";
        case TokenType::KEYWORD_IMPORT: return "import";
        case TokenType::KEYWORD_INT: return "int";
        case TokenType::KEYWORD_FLOAT: return "float";
        case TokenType::KEYWORD_BOOL: return "bool";
        case TokenType::KEYWORD_FOR: return "for";
        case TokenType::KEYWORD_WINDOW: return "window";
        case TokenType::PUNCTUATOR_LPAREN: return "(";
        case TokenType::PUNCTUATOR_RPAREN: return ")";
        case TokenType::PUNCTUATOR_LBRACE: return "{";
        case TokenType::PUNCTUATOR_RBRACE: return "}";
        case TokenType::PUNCTUATOR_LBRACKET: return "[";
        case TokenType::PUNCTUATOR_RBRACKET: return "]";
        case TokenType::PUNCTUATOR_SEMICOLON: return ";";
        case TokenType::PUNCTUATOR_COMMA: return ",";
        case TokenType::PUNCTUATOR_PLUS: return "+";
        case TokenType::PUNCTUATOR_MINUS: return "-";
        case TokenType::PUNCTUATOR_STAR: return "*";
        case TokenType::PUNCTUATOR_SLASH: return "/";
        case TokenType::PUNCTUATOR_PERCENT: return "%";
        case TokenType::PUNCTUATOR_EQUAL: return "=";
        case TokenType::PUNCTUATOR_EQUAL_EQUAL: return "==";
        case TokenType::PUNCTUATOR_EXCLAIM: return "!";
        case TokenType::PUNCTUATOR_EXCLAIM_EQUAL: return "!=";
        case TokenType::PUNCTUATOR_LESS: return "<";
        case TokenType::PUNCTUATOR_LESS_EQUAL: return "<=";
        case TokenType::PUNCTUATOR_GREATER: return ">";
        case TokenType::PUNCTUATOR_GREATER_EQUAL: return ">=";
        case TokenType::PUNCTUATOR_AND: return "&&";
        case TokenType::PUNCTUATOR_OR: return "||";
        case TokenType::PUNCTUATOR_HASH: return "#";
        case TokenType::PUNCTUATOR_DOLLAR: return "$";
        case TokenType::PUNCTUATOR_COLON: return ":";
        case TokenType::PUNCTUATOR_DOT_DOT_DOT: return "...";
        default: return "unknown";
    }
}

Lexer::Lexer(const std::string& source) 
    : source(source), current_pos(0), line(1), column(1) {}

bool Lexer::isAtEnd() {
    return current_pos >= source.length();
}

char Lexer::peek(int offset) {
    size_t pos = current_pos + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    char c = source[current_pos];
    current_pos++;
    column++;
    if (c == '\n') {
        line++;
        column = 1;
    }
    return c;
}

void Lexer::skipWhiteSpace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            advance();
        } else if (c == '\\' && peek(1) == '\n') {
            advance();
            advance();
        } else {
            return;
        }
    }
}

void Lexer::skipComment() {
    if (peek() == '/' && peek(1) == '/') {
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    } else if (peek() == '/' && peek(1) == '*') {
        advance();
        advance();
        while (!isAtEnd()) {
            if (peek() == '*' && peek(1) == '/') {
                advance();
                advance();
                break;
            } else {
                advance();
            }
        }
    }
}

void Lexer::addToken(TokenType type, const std::string& text) {
        tokens.push_back(Token(type, text, SourceLocation(line, column, text.length())));
    }

void Lexer::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(line) + ", column " + std::to_string(column));
}

void Lexer::lexIdentifier() {
    int start_column = column;
    std::string text;
    while (!isAtEnd() && (isAlphaNumeric(peek()) || peek() == '_')) {
        text += advance();
    }
    
    auto it = std::find_if(keywords.begin(), keywords.end(), 
        [&text](const std::pair<std::string, TokenType>& pair) { 
            return pair.first == text; 
        });
    
    if (it != keywords.end()) {
        addToken(it->second, text);
    } else {
        addToken(TokenType::IDENTIFIER, text);
    }
}

void Lexer::lexNumber() {
    int start_column = column;
    std::string text;
    bool isFloat = false;
    
    while (!isAtEnd() && std::isdigit(peek())) {
        text += advance();
    }
    
    if (!isAtEnd() && peek() == '.' && std::isdigit(peek(1))) {
        isFloat = true;
        text += advance();
        while (!isAtEnd() && std::isdigit(peek())) {
            text += advance();
        }
    }
    
    if (isFloat) {
        addToken(TokenType::FLOAT_LITERAL, text);
    } else {
        try {
            unsigned long long value = std::stoull(text);
            constexpr unsigned long long INT32_MAX_VAL = 2147483647ULL;
            
            if (value > INT32_MAX_VAL) {
                addError("Integer literal '" + text + "' out of range (valid range: 0 to 2147483647 for positive, -2147483648 to -1 via unary minus)");
                addToken(TokenType::NUMBER_LITERAL, text);
            } else {
                addToken(TokenType::NUMBER_LITERAL, text);
            }
        } catch (const std::out_of_range&) {
            addError("Integer literal '" + text + "' out of range (valid range: 0 to 2147483647 for positive, -2147483648 to -1 via unary minus)");
            addToken(TokenType::NUMBER_LITERAL, text);
        }
    }
}

void Lexer::lexString() {
    int start_column = column;
    std::string text;
    advance();
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            text += advance();
            if (!isAtEnd()) {
                text += advance();
            }
        } else {
            text += advance();
        }
    }
    if (!isAtEnd()) {
        advance();
    }
    addToken(TokenType::STRING_LITERAL, text);
}

void Lexer::lexPunctuatuator() {
    int start_column = column;
    std::string text;
    
    for (int len = 3; len >= 1; len--) {
        text.clear();
        for (int i = 0; i < len && !isAtEnd(); i++) {
            text += peek(i);
        }
        
        auto it = std::find_if(punctuatuators.begin(), punctuatuators.end(), 
            [&text](const std::pair<std::string, TokenType>& pair) { 
                return pair.first == text; 
            });
        
        if (it != punctuatuators.end()) {
            for (int i = 0; i < len; i++) {
                advance();
            }
            addToken(it->second, text);
            return;
        }
    }
    
    text = advance();
    addError("Unexpected punctuator: " + text);
}

std::vector<Token> Lexer::tokenize() {
    tokens.clear();
    errors.clear();
    current_pos = 0;
    line = 1;
    column = 1;
    
    while (!isAtEnd()) {
        skipWhiteSpace();
        if (isAtEnd()) break;
        
        char c = peek();
        
        if (c == '/' && (peek(1) == '/' || peek(1) == '*')) {
            skipComment();
            continue;
        }
        
        if (c == '%' &&
            peek(1) == 'i' &&
            peek(2) == 'm' &&
            peek(3) == 'p' &&
            peek(4) == 'o' &&
            peek(5) == 'r' &&
            peek(6) == 't' &&
            !isAlphaNumeric(peek(7)) &&
            peek(7) != '_') {
            advance();
            std::string text;
            while (!isAtEnd() && (isAlphaNumeric(peek()) || peek() == '_')) {
                text += advance();
            }
            addToken(TokenType::KEYWORD_IMPORT, text);
            continue;
        }
        
        if (isAlpha(c)) {
            lexIdentifier();
        } else if (isDigit(c)) {
            lexNumber();
        } else if (c == '"') {
            lexString();
        } else {
            lexPunctuatuator();
        }
    }
    
    SourceLocation loc(line, column, 0);
    tokens.push_back(Token(TokenType::END_OF_FILE, "", loc));
    
    return tokens;
}

}
