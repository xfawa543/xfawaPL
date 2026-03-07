#include "xfawa_mods_lexer.h"
#include <vector>
#include <cctype>
#include <algorithm>

namespace xfawa {

const std::vector<std::pair<std::string, ModsTokenType>> ModsLexer::keywords = {
    {"fn", ModsTokenType::KEYWORD_FN},
    {"if", ModsTokenType::KEYWORD_IF},
    {"else", ModsTokenType::KEYWORD_ELSE},
    {"while", ModsTokenType::KEYWORD_WHILE},
    {"break", ModsTokenType::KEYWORD_BREAK},
    {"return", ModsTokenType::KEYWORD_RETURN},
    {"true", ModsTokenType::KEYWORD_TRUE},
    {"false", ModsTokenType::KEYWORD_FALSE},
    {"print", ModsTokenType::KEYWORD_PRINT},
    {"import", ModsTokenType::KEYWORD_IMPORT}
};

const std::vector<std::pair<std::string, ModsTokenType>> ModsLexer::punctuators = {
    {"(", ModsTokenType::PUNCTUATOR_LPAREN},
    {")", ModsTokenType::PUNCTUATOR_RPAREN},
    {"{", ModsTokenType::PUNCTUATOR_LBRACE},
    {"}", ModsTokenType::PUNCTUATOR_RBRACE},
    {"[", ModsTokenType::PUNCTUATOR_LBRACKET},
    {"]", ModsTokenType::PUNCTUATOR_RBRACKET},
    {";", ModsTokenType::PUNCTUATOR_SEMICOLON},
    {",", ModsTokenType::PUNCTUATOR_COMMA},
    {"+", ModsTokenType::PUNCTUATOR_PLUS},
    {"-", ModsTokenType::PUNCTUATOR_MINUS},
    {"*", ModsTokenType::PUNCTUATOR_STAR},
    {"/", ModsTokenType::PUNCTUATOR_SLASH},
    {"%", ModsTokenType::PUNCTUATOR_PERCENT},
    {"=", ModsTokenType::PUNCTUATOR_EQUAL},
    {"==", ModsTokenType::PUNCTUATOR_EQUAL_EQUAL},
    {"!", ModsTokenType::PUNCTUATOR_EXCLAIM},
    {"!=", ModsTokenType::PUNCTUATOR_EXCLAIM_EQUAL},
    {"<", ModsTokenType::PUNCTUATOR_LESS},
    {"<=", ModsTokenType::PUNCTUATOR_LESS_EQUAL},
    {">", ModsTokenType::PUNCTUATOR_GREATER},
    {">=", ModsTokenType::PUNCTUATOR_GREATER_EQUAL},
    {"&&", ModsTokenType::PUNCTUATOR_AND},
    {"||", ModsTokenType::PUNCTUATOR_OR},
    {"#", ModsTokenType::PUNCTUATOR_HASH},
    {"$", ModsTokenType::PUNCTUATOR_DOLLAR}
};

std::string ModsToken::toString() const {
    std::string typeStr;
    switch (type) {
        case ModsTokenType::END_OF_FILE: typeStr = "EOF"; break;
        case ModsTokenType::IDENTIFIER: typeStr = "identifier"; break;
        case ModsTokenType::NUMBER_LITERAL: typeStr = "number"; break;
        case ModsTokenType::STRING_LITERAL: typeStr = "string"; break;
        case ModsTokenType::KEYWORD_FN: typeStr = "fn"; break;
        case ModsTokenType::KEYWORD_IF: typeStr = "if"; break;
        case ModsTokenType::KEYWORD_ELSE: typeStr = "else"; break;
        case ModsTokenType::KEYWORD_WHILE: typeStr = "while"; break;
        case ModsTokenType::KEYWORD_BREAK: typeStr = "break"; break;
        case ModsTokenType::KEYWORD_RETURN: typeStr = "return"; break;
        case ModsTokenType::KEYWORD_TRUE: typeStr = "true"; break;
        case ModsTokenType::KEYWORD_FALSE: typeStr = "false"; break;
        case ModsTokenType::KEYWORD_PRINT: typeStr = "print"; break;
        case ModsTokenType::KEYWORD_IMPORT: typeStr = "import"; break;
        case ModsTokenType::PUNCTUATOR_LPAREN: typeStr = "("; break;
        case ModsTokenType::PUNCTUATOR_RPAREN: typeStr = ")"; break;
        case ModsTokenType::PUNCTUATOR_LBRACE: typeStr = "{"; break;
        case ModsTokenType::PUNCTUATOR_RBRACE: typeStr = "}"; break;
        case ModsTokenType::PUNCTUATOR_LBRACKET: typeStr = "["; break;
        case ModsTokenType::PUNCTUATOR_RBRACKET: typeStr = "]"; break;
        case ModsTokenType::PUNCTUATOR_SEMICOLON: typeStr = ";"; break;
        case ModsTokenType::PUNCTUATOR_COMMA: typeStr = ","; break;
        case ModsTokenType::PUNCTUATOR_PLUS: typeStr = "+"; break;
        case ModsTokenType::PUNCTUATOR_MINUS: typeStr = "-"; break;
        case ModsTokenType::PUNCTUATOR_STAR: typeStr = "*"; break;
        case ModsTokenType::PUNCTUATOR_SLASH: typeStr = "/"; break;
        case ModsTokenType::PUNCTUATOR_PERCENT: typeStr = "%"; break;
        case ModsTokenType::PUNCTUATOR_EQUAL: typeStr = "="; break;
        case ModsTokenType::PUNCTUATOR_EQUAL_EQUAL: typeStr = "=="; break;
        case ModsTokenType::PUNCTUATOR_EXCLAIM: typeStr = "!"; break;
        case ModsTokenType::PUNCTUATOR_EXCLAIM_EQUAL: typeStr = "!="; break;
        case ModsTokenType::PUNCTUATOR_LESS: typeStr = "<"; break;
        case ModsTokenType::PUNCTUATOR_LESS_EQUAL: typeStr = "<="; break;
        case ModsTokenType::PUNCTUATOR_GREATER: typeStr = ">"; break;
        case ModsTokenType::PUNCTUATOR_GREATER_EQUAL: typeStr = ">="; break;
        case ModsTokenType::PUNCTUATOR_AND: typeStr = "&&"; break;
        case ModsTokenType::PUNCTUATOR_OR: typeStr = "||"; break;
        case ModsTokenType::PUNCTUATOR_HASH: typeStr = "#"; break;
        case ModsTokenType::PUNCTUATOR_DOLLAR: typeStr = "$"; break;
        default: typeStr = "unknown"; break;
    }
    return typeStr + " " + text + " (" + std::to_string(line) + ":" + std::to_string(column) + ")";
}

ModsLexer::ModsLexer(const std::string& source) 
    : source(source), current_pos(0), line(1), column(1) {}

bool ModsLexer::isAtEnd() {
    return current_pos >= source.length();
}

char ModsLexer::peek(int offset) {
    size_t pos = current_pos + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char ModsLexer::advance() {
    char c = source[current_pos];
    current_pos++;
    column++;
    if (c == '\n') {
        line++;
        column = 1;
    }
    return c;
}

void ModsLexer::skipWhiteSpace() {
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

void ModsLexer::skipComment() {
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

void ModsLexer::addToken(ModsTokenType type, const std::string& text) {
    tokens.push_back(ModsToken(type, text, line, column));
}

void ModsLexer::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(line) + ", column " + std::to_string(column));
}

void ModsLexer::lexIdentifier() {
    int start_column = column;
    std::string text;
    while (!isAtEnd() && (isAlphaNumeric(peek()) || peek() == '_')) {
        text += advance();
    }
    
    auto it = std::find_if(keywords.begin(), keywords.end(), 
        [&text](const std::pair<std::string, ModsTokenType>& pair) { 
            return pair.first == text; 
        });
    
    if (it != keywords.end()) {
        addToken(it->second, text);
    } else {
        addToken(ModsTokenType::IDENTIFIER, text);
    }
}

void ModsLexer::lexNumber() {
    int start_column = column;
    std::string text;
    while (!isAtEnd() && std::isdigit(peek())) {
        text += advance();
    }
    addToken(ModsTokenType::NUMBER_LITERAL, text);
}

void ModsLexer::lexString() {
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
    addToken(ModsTokenType::STRING_LITERAL, text);
}

void ModsLexer::lexPunctuator() {
    int start_column = column;
    std::string text;
    
    text += advance();
    
    while (!isAtEnd()) {
        std::string two_char = text + peek();
        bool found = false;
        for (const auto& punctuator : punctuators) {
            if (two_char == punctuator.first) {
                found = true;
                break;
            }
        }
        if (found) {
            text += advance();
        } else {
            break;
        }
    }
    
    auto it = std::find_if(punctuators.begin(), punctuators.end(), 
        [&text](const std::pair<std::string, ModsTokenType>& pair) { 
            return pair.first == text; 
        });
    
    if (it != punctuators.end()) {
        addToken(it->second, text);
    } else {
        addError("Unexpected punctuator: " + text);
    }
}

std::vector<ModsToken> ModsLexer::tokenize() {
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
        
        if (c == '%') {
            advance();
            if (peek() == 'i' || peek() == 'I') {
                lexIdentifier();
            } else {
                addError("Expected 'import' after '%'");
                advance();
            }
            continue;
        }
        
        if (isAlpha(c)) {
            lexIdentifier();
        } else if (isDigit(c)) {
            lexNumber();
        } else if (c == '"') {
            lexString();
        } else {
            lexPunctuator();
        }
    }
    
    tokens.push_back(ModsToken(ModsTokenType::END_OF_FILE, "", line, column));
    
    return tokens;
}

}