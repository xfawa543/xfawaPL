#include "xfawa_mods_system.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace xfawa {

extern int g_debug_global;

const std::vector<std::pair<std::string, ModTokenType>> ModLexer::keywords = {
    {"fn", ModTokenType::KEYWORD_FN},
    {"if", ModTokenType::KEYWORD_IF},
    {"else", ModTokenType::KEYWORD_ELSE},
    {"while", ModTokenType::KEYWORD_WHILE},
    {"break", ModTokenType::KEYWORD_BREAK},
    {"return", ModTokenType::KEYWORD_RETURN},
    {"true", ModTokenType::KEYWORD_TRUE},
    {"false", ModTokenType::KEYWORD_FALSE},
    {"print", ModTokenType::KEYWORD_PRINT},
    {"import", ModTokenType::KEYWORD_IMPORT},
    {"name", ModTokenType::KEYWORD_NAME},
    {"syntax", ModTokenType::KEYWORD_SYNTAX},
    {"logic", ModTokenType::KEYWORD_LOGIC},
    {"action", ModTokenType::KEYWORD_ACTION}
};

const std::vector<std::pair<std::string, ModTokenType>> ModLexer::punctuators = {
    {"(", ModTokenType::PUNCTUATOR_LPAREN},
    {")", ModTokenType::PUNCTUATOR_RPAREN},
    {"{", ModTokenType::PUNCTUATOR_LBRACE},
    {"}", ModTokenType::PUNCTUATOR_RBRACE},
    {"[", ModTokenType::PUNCTUATOR_LBRACKET},
    {"]", ModTokenType::PUNCTUATOR_RBRACKET},
    {";", ModTokenType::PUNCTUATOR_SEMICOLON},
    {",", ModTokenType::PUNCTUATOR_COMMA},
    {":", ModTokenType::PUNCTUATOR_COLON},
    {"+", ModTokenType::PUNCTUATOR_PLUS},
    {"-", ModTokenType::PUNCTUATOR_MINUS},
    {"*", ModTokenType::PUNCTUATOR_STAR},
    {"/", ModTokenType::PUNCTUATOR_SLASH},
    {"%", ModTokenType::PUNCTUATOR_PERCENT},
    {"=", ModTokenType::PUNCTUATOR_EQUAL},
    {"==", ModTokenType::PUNCTUATOR_EQUAL_EQUAL},
    {"!", ModTokenType::PUNCTUATOR_EXCLAIM},
    {"!=", ModTokenType::PUNCTUATOR_EXCLAIM_EQUAL},
    {"<", ModTokenType::PUNCTUATOR_LESS},
    {"<=", ModTokenType::PUNCTUATOR_LESS_EQUAL},
    {">", ModTokenType::PUNCTUATOR_GREATER},
    {">=", ModTokenType::PUNCTUATOR_GREATER_EQUAL},
    {"<<", ModTokenType::PUNCTUATOR_LESS_LESS},
    {">>", ModTokenType::PUNCTUATOR_GREATER_GREATER},
    {"&&", ModTokenType::PUNCTUATOR_AND},
    {"||", ModTokenType::PUNCTUATOR_OR},
    {"#", ModTokenType::PUNCTUATOR_HASH},
    {"$", ModTokenType::PUNCTUATOR_DOLLAR},
    {"@", ModTokenType::PUNCTUATOR_AT},
    {"=>", ModTokenType::PUNCTUATOR_ARROW},
    {"->", ModTokenType::PUNCTUATOR_FAT_ARROW},
    {"...", ModTokenType::PUNCTUATOR_DOT_DOT_DOT}
};

std::string ModToken::toString() const {
    std::string typeStr;
    switch (type) {
        case ModTokenType::END_OF_FILE: typeStr = "EOF"; break;
        case ModTokenType::IDENTIFIER: typeStr = "identifier"; break;
        case ModTokenType::NUMBER_LITERAL: typeStr = "number"; break;
        case ModTokenType::STRING_LITERAL: typeStr = "string"; break;
        case ModTokenType::KEYWORD_FN: typeStr = "fn"; break;
        case ModTokenType::KEYWORD_IF: typeStr = "if"; break;
        case ModTokenType::KEYWORD_ELSE: typeStr = "else"; break;
        case ModTokenType::KEYWORD_WHILE: typeStr = "while"; break;
        case ModTokenType::KEYWORD_BREAK: typeStr = "break"; break;
        case ModTokenType::KEYWORD_RETURN: typeStr = "return"; break;
        case ModTokenType::KEYWORD_TRUE: typeStr = "true"; break;
        case ModTokenType::KEYWORD_FALSE: typeStr = "false"; break;
        case ModTokenType::KEYWORD_PRINT: typeStr = "print"; break;
        case ModTokenType::KEYWORD_IMPORT: typeStr = "import"; break;
        case ModTokenType::KEYWORD_NAME: typeStr = "name"; break;
        case ModTokenType::KEYWORD_SYNTAX: typeStr = "syntax"; break;
        case ModTokenType::KEYWORD_LOGIC: typeStr = "logic"; break;
        case ModTokenType::KEYWORD_ACTION: typeStr = "action"; break;
        case ModTokenType::PUNCTUATOR_LPAREN: typeStr = "("; break;
        case ModTokenType::PUNCTUATOR_RPAREN: typeStr = ")"; break;
        case ModTokenType::PUNCTUATOR_LBRACE: typeStr = "{"; break;
        case ModTokenType::PUNCTUATOR_RBRACE: typeStr = "}"; break;
        case ModTokenType::PUNCTUATOR_LBRACKET: typeStr = "["; break;
        case ModTokenType::PUNCTUATOR_RBRACKET: typeStr = "]"; break;
        case ModTokenType::PUNCTUATOR_SEMICOLON: typeStr = ";"; break;
        case ModTokenType::PUNCTUATOR_COMMA: typeStr = ","; break;
        case ModTokenType::PUNCTUATOR_COLON: typeStr = ":"; break;
        case ModTokenType::PUNCTUATOR_PLUS: typeStr = "+"; break;
        case ModTokenType::PUNCTUATOR_MINUS: typeStr = "-"; break;
        case ModTokenType::PUNCTUATOR_STAR: typeStr = "*"; break;
        case ModTokenType::PUNCTUATOR_SLASH: typeStr = "/"; break;
        case ModTokenType::PUNCTUATOR_PERCENT: typeStr = "%"; break;
        case ModTokenType::PUNCTUATOR_EQUAL: typeStr = "="; break;
        case ModTokenType::PUNCTUATOR_EQUAL_EQUAL: typeStr = "=="; break;
        case ModTokenType::PUNCTUATOR_EXCLAIM: typeStr = "!"; break;
        case ModTokenType::PUNCTUATOR_EXCLAIM_EQUAL: typeStr = "!="; break;
        case ModTokenType::PUNCTUATOR_LESS: typeStr = "<"; break;
        case ModTokenType::PUNCTUATOR_LESS_EQUAL: typeStr = "<="; break;
        case ModTokenType::PUNCTUATOR_GREATER: typeStr = ">"; break;
        case ModTokenType::PUNCTUATOR_GREATER_EQUAL: typeStr = ">="; break;
        case ModTokenType::PUNCTUATOR_LESS_LESS: typeStr = "<<"; break;
        case ModTokenType::PUNCTUATOR_GREATER_GREATER: typeStr = ">>"; break;
        case ModTokenType::PUNCTUATOR_AND: typeStr = "&&"; break;
        case ModTokenType::PUNCTUATOR_OR: typeStr = "||"; break;
        case ModTokenType::PUNCTUATOR_HASH: typeStr = "#"; break;
        case ModTokenType::PUNCTUATOR_DOLLAR: typeStr = "$"; break;
        case ModTokenType::PUNCTUATOR_AT: typeStr = "@"; break;
        case ModTokenType::PUNCTUATOR_ARROW: typeStr = "=>"; break;
        case ModTokenType::PUNCTUATOR_FAT_ARROW: typeStr = "->"; break;
        case ModTokenType::PUNCTUATOR_DOT_DOT_DOT: typeStr = "..."; break;
        default: typeStr = "unknown"; break;
    }
    return typeStr + " '" + text + "' (" + std::to_string(line) + ":" + std::to_string(column) + ")";
}

ModLexer::ModLexer(const std::string& source) 
    : source(source), current_pos(0), line(1), column(1) {}

bool ModLexer::isAtEnd() {
    return current_pos >= source.length();
}

char ModLexer::peek(int offset) {
    size_t pos = current_pos + offset;
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char ModLexer::advance() {
    char c = source[current_pos];
    current_pos++;
    column++;
    if (c == '\n') {
        line++;
        column = 1;
    }
    return c;
}

void ModLexer::skipWhiteSpace() {
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

void ModLexer::skipComment() {
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

void ModLexer::addToken(ModTokenType type, const std::string& text) {
    tokens.push_back(ModToken(type, text, line, column));
}

void ModLexer::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(line) + ", column " + std::to_string(column));
}

void ModLexer::lexIdentifier() {
    std::string text;
    while (!isAtEnd() && (isAlphaNumeric(peek()) || peek() == '_')) {
        text += advance();
    }
    
    auto it = std::find_if(keywords.begin(), keywords.end(), 
        [&text](const std::pair<std::string, ModTokenType>& pair) { 
            return pair.first == text; 
        });
    
    if (it != keywords.end()) {
        addToken(it->second, text);
    } else {
        addToken(ModTokenType::IDENTIFIER, text);
    }
}

void ModLexer::lexNumber() {
    std::string text;
    while (!isAtEnd() && std::isdigit(peek())) {
        text += advance();
    }
    addToken(ModTokenType::NUMBER_LITERAL, text);
}

void ModLexer::lexString() {
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
    addToken(ModTokenType::STRING_LITERAL, text);
}

void ModLexer::lexPunctuator() {
    std::string text;
    
    for (int len = 3; len >= 1; len--) {
        text.clear();
        for (int i = 0; i < len && !isAtEnd(); i++) {
            text += peek(i);
        }
        
        auto it = std::find_if(punctuators.begin(), punctuators.end(), 
            [&text](const std::pair<std::string, ModTokenType>& pair) { 
                return pair.first == text; 
            });
        
        if (it != punctuators.end()) {
            for (int i = 0; i < len; i++) {
                advance();
            }
            addToken(it->second, text);
            return;
        }
    }
    
    addError("Unexpected punctuator: " + std::string(1, peek()));
    advance();
}

std::vector<ModToken> ModLexer::tokenize() {
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
    
    tokens.push_back(ModToken(ModTokenType::END_OF_FILE, "", line, column));
    
    return tokens;
}

ModParser::ModParser(const std::vector<ModToken>& toks) : tokens(toks), current(0) {}

const ModToken& ModParser::peek() const {
    if (current < tokens.size()) {
        return tokens[current];
    }
    static ModToken eofToken;
    return eofToken;
}

const ModToken& ModParser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    static ModToken eofToken;
    return eofToken;
}

ModToken ModParser::consume() {
    if (current < tokens.size()) {
        return tokens[current++];
    }
    return ModToken();
}

bool ModParser::consume(ModTokenType type) {
    if (peek().is(type)) {
        current++;
        return true;
    }
    return false;
}

bool ModParser::isAtEnd() const {
    return peek().is(ModTokenType::END_OF_FILE);
}

void ModParser::advance() {
    if (current < tokens.size()) {
        current++;
    }
}

void ModParser::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(peek().line));
}

bool ModParser::parse() {
    while (!isAtEnd()) {
        if (peek().is(ModTokenType::PUNCTUATOR_HASH)) {
            if (!parseBlock()) {
                return false;
            }
        } else {
            advance();
        }
    }
    return true;
}

bool ModParser::parseBlock() {
    if (!consume(ModTokenType::PUNCTUATOR_HASH)) {
        addError("Expected '#'");
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER)) {
        addError("Expected block name after '#'");
        return false;
    }
    
    std::string blockName = tokens[current - 1].text;
    ModBlock block(blockName);
    
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after block name");
        return false;
    }
    
    if (!parseBlockContent(block)) {
        return false;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close block");
        return false;
    }
    
    blocks.push_back(std::move(block));
    return true;
}

bool ModParser::parseBlockContent(ModBlock& block) {
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
        if (peek().is(ModTokenType::KEYWORD_FN)) {
            if (!parseFunction(block)) {
                return false;
            }
        } else if (peek().is(ModTokenType::PUNCTUATOR_AT)) {
            if (!parseAddCommand(block)) {
                return false;
            }
        } else if (peek().is(ModTokenType::STRING_LITERAL)) {
            if (!parseModification(block)) {
                return false;
            }
        } else {
            addError("Unexpected token in block: " + peek().toString());
            advance();
        }
    }
    return true;
}

bool ModParser::parseFunction(ModBlock& block) {
    if (!consume(ModTokenType::KEYWORD_FN)) {
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER)) {
        addError("Expected function name after 'fn'");
        return false;
    }
    
    std::string ns;
    std::string funcName = tokens[current - 1].text;
    
    if (consume(ModTokenType::PUNCTUATOR_COLON)) {
        ns = funcName;
        if (!consume(ModTokenType::IDENTIFIER)) {
            addError("Expected function name after namespace ':'");
            return false;
        }
        funcName = tokens[current - 1].text;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after function name");
        return false;
    }
    
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RPAREN)) {
        advance();
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after function parameters");
        return false;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' to start function body");
        return false;
    }
    
    int braceCount = 1;
    while (!isAtEnd() && braceCount > 0) {
        if (peek().is(ModTokenType::PUNCTUATOR_LBRACE)) {
            braceCount++;
        } else if (peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
            braceCount--;
        }
        if (braceCount > 0) {
            if (peek().is(ModTokenType::STRING_LITERAL)) {
                parseModification(block);
            } else if (peek().is(ModTokenType::KEYWORD_FN)) {
                if (peek(1).is(ModTokenType::IDENTIFIER) && peek(2).is(ModTokenType::PUNCTUATOR_COLON)) {
                    PublicFunction pubFunc;
                    pubFunc.line = peek().line;
                    if (parseNestedPublicFunction(pubFunc)) {
                        block.publicFunctions.push_back(pubFunc);
                    }
                } else {
                    PublicFunction internalFunc;
                    internalFunc.line = peek().line;
                    internalFunc.ns = "";
                    std::string funcName = peek(1).text;
                    if (parseNestedInternalFunction(internalFunc)) {
                        block.publicFunctions.push_back(internalFunc);
                        block.internalFunctionNames.insert(funcName);
                    }
                }
            } else {
                advance();
            }
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close function body");
        return false;
    }
    
    return true;
}

bool ModParser::parseNestedPublicFunction(PublicFunction& pubFunc) {
    if (!consume(ModTokenType::KEYWORD_FN)) {
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER)) {
        addError("Expected namespace after 'fn' in public function");
        return false;
    }
    
    std::string ns = tokens[current - 1].text;
    
    if (!consume(ModTokenType::PUNCTUATOR_COLON)) {
        addError("Expected ':' after namespace in public function");
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER)) {
        addError("Expected function name after namespace ':'");
        return false;
    }
    
    std::string funcName = tokens[current - 1].text;
    
    if (!consume(ModTokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after function name");
        return false;
    }
    
    std::vector<std::string> params;
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RPAREN)) {
        if (peek().is(ModTokenType::IDENTIFIER)) {
            params.push_back(peek().text);
        }
        advance();
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after function parameters");
        return false;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' to start function body");
        return false;
    }
    
    int braceCount = 1;
    int bodyStart = current;
    while (!isAtEnd() && braceCount > 0) {
        if (peek().is(ModTokenType::PUNCTUATOR_LBRACE)) {
            braceCount++;
        } else if (peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
            braceCount--;
        }
        if (braceCount > 0) {
            advance();
        }
    }
    
    std::string body;
    for (int i = bodyStart; i < current; i++) {
        if (tokens[i].is(ModTokenType::STRING_LITERAL)) {
            body += "\"" + tokens[i].text + "\"";
        } else {
            body += tokens[i].text;
        }
        
        if (tokens[i].is(ModTokenType::PUNCTUATOR_LBRACE)) {
            body += "\n";
        } else if (tokens[i].is(ModTokenType::PUNCTUATOR_RBRACE)) {
            body += "\n";
        } else if (tokens[i].is(ModTokenType::PUNCTUATOR_SEMICOLON)) {
            body += "\n";
        } else if (i + 1 < static_cast<int>(tokens.size())) {
            auto& nextToken = tokens[i + 1];
            if (nextToken.is(ModTokenType::KEYWORD_IF) ||
                nextToken.is(ModTokenType::KEYWORD_ELSE) ||
                nextToken.is(ModTokenType::KEYWORD_WHILE) ||
                nextToken.is(ModTokenType::KEYWORD_PRINT) ||
                nextToken.is(ModTokenType::KEYWORD_FN)) {
                body += "\n";
            } else if (nextToken.is(ModTokenType::PUNCTUATOR_RBRACE) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_LBRACE) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_RPAREN) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_LPAREN) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_COMMA) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_COLON)) {
            } else {
                body += " ";
            }
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close function body");
        return false;
    }
    
    pubFunc.ns = ns;
    pubFunc.name = funcName;
    pubFunc.params = params;
    pubFunc.body = body;
    
    return true;
}

bool ModParser::parseNestedInternalFunction(PublicFunction& internalFunc) {
    if (!consume(ModTokenType::KEYWORD_FN)) {
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER)) {
        addError("Expected function name after 'fn'");
        return false;
    }
    
    std::string funcName = tokens[current - 1].text;
    
    if (!consume(ModTokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after function name");
        return false;
    }
    
    std::vector<std::string> params;
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RPAREN)) {
        if (peek().is(ModTokenType::IDENTIFIER)) {
            params.push_back(peek().text);
        }
        advance();
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after function parameters");
        return false;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' to start function body");
        return false;
    }
    
    int braceCount = 1;
    int bodyStart = current;
    while (!isAtEnd() && braceCount > 0) {
        if (peek().is(ModTokenType::PUNCTUATOR_LBRACE)) {
            braceCount++;
        } else if (peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
            braceCount--;
        }
        if (braceCount > 0) {
            advance();
        }
    }
    
    std::string body;
    for (int i = bodyStart; i < current; i++) {
        if (tokens[i].is(ModTokenType::STRING_LITERAL)) {
            body += "\"" + tokens[i].text + "\"";
        } else {
            body += tokens[i].text;
        }
        
        if (tokens[i].is(ModTokenType::PUNCTUATOR_LBRACE)) {
            body += "\n";
        } else if (tokens[i].is(ModTokenType::PUNCTUATOR_RBRACE)) {
            body += "\n";
        } else if (tokens[i].is(ModTokenType::PUNCTUATOR_SEMICOLON)) {
            body += "\n";
        } else if (i + 1 < static_cast<int>(tokens.size())) {
            auto& nextToken = tokens[i + 1];
            if (nextToken.is(ModTokenType::KEYWORD_IF) ||
                nextToken.is(ModTokenType::KEYWORD_ELSE) ||
                nextToken.is(ModTokenType::KEYWORD_WHILE) ||
                nextToken.is(ModTokenType::KEYWORD_PRINT) ||
                nextToken.is(ModTokenType::KEYWORD_FN)) {
                body += "\n";
            } else if (nextToken.is(ModTokenType::PUNCTUATOR_RBRACE) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_LBRACE) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_RPAREN) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_LPAREN) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_COMMA) ||
                       nextToken.is(ModTokenType::PUNCTUATOR_COLON)) {
            } else {
                body += " ";
            }
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close function body");
        return false;
    }
    
    internalFunc.ns = "";
    internalFunc.name = funcName;
    internalFunc.params = params;
    internalFunc.body = body;
    
    return true;
}

bool ModParser::parseAddCommand(ModBlock& block) {
    if (!consume(ModTokenType::PUNCTUATOR_AT)) {
        return false;
    }
    
    if (!consume(ModTokenType::IDENTIFIER) || tokens[current - 1].text != "add") {
        addError("Expected 'add' after '@'");
        return false;
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after '@add'");
        return false;
    }
    
    AddedSyntax syntax;
    syntax.line = peek().line;
    
    if (g_debug_global) {
        std::cerr << "[mods debug] Starting parseAddCommand" << std::endl;
    }
    
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
        if (g_debug_global) {
            std::cerr << "[mods debug] Processing token: " << peek().toString() << std::endl;
        }
        
        if (peek().is(ModTokenType::KEYWORD_NAME)) {
            advance();
            if (!consume(ModTokenType::PUNCTUATOR_COLON)) {
                addError("Expected ':' after 'name'");
                return false;
            }
            if (!consume(ModTokenType::IDENTIFIER)) {
                addError("Expected identifier after 'name:'");
                return false;
            }
            syntax.name = tokens[current - 1].text;
        } else if (peek().is(ModTokenType::KEYWORD_SYNTAX)) {
            advance();
            if (!consume(ModTokenType::PUNCTUATOR_COLON)) {
                addError("Expected ':' after 'syntax'");
                return false;
            }
            
            std::string syntaxPattern;
            while (!isAtEnd() && !peek().is(ModTokenType::KEYWORD_LOGIC) && 
                   !peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
                if (peek().is(ModTokenType::IDENTIFIER) || 
                    peek().is(ModTokenType::NUMBER_LITERAL) ||
                    peek().is(ModTokenType::STRING_LITERAL) ||
                    peek().is(ModTokenType::KEYWORD_ACTION)) {
                    syntaxPattern += tokens[current].text + " ";
                    advance();
                } else if (peek().is(ModTokenType::PUNCTUATOR_PERCENT)) {
                    syntaxPattern += "% ";
                    advance();
                } else if (peek().is(ModTokenType::PUNCTUATOR_LESS)) {
                    syntaxPattern += "<";
                    advance();
                    if (peek().is(ModTokenType::IDENTIFIER)) {
                        syntaxPattern += tokens[current].text;
                        advance();
                    }
                    if (peek().is(ModTokenType::PUNCTUATOR_GREATER)) {
                        syntaxPattern += "> ";
                        advance();
                    }
                } else if (peek().is(ModTokenType::PUNCTUATOR_LBRACE)) {
                    syntaxPattern += "{ ";
                    advance();
                    if (peek().is(ModTokenType::PUNCTUATOR_LESS)) {
                        syntaxPattern += "<";
                        advance();
                        if (peek().is(ModTokenType::IDENTIFIER)) {
                            syntaxPattern += tokens[current].text;
                            syntax.parameters.push_back(SyntaxParameter(tokens[current].text, true));
                            advance();
                        } else if (peek().is(ModTokenType::KEYWORD_ACTION)) {
                            syntaxPattern += tokens[current].text;
                            syntax.parameters.push_back(SyntaxParameter(tokens[current].text, true));
                            advance();
                        }
                        if (peek().is(ModTokenType::PUNCTUATOR_GREATER)) {
                            syntaxPattern += "> ";
                            advance();
                        }
                    }
                    if (peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
                        syntaxPattern += "} ";
                        advance();
                    }
                } else {
                    advance();
                }
            }
            syntax.syntaxPattern = syntaxPattern;
        } else if (peek().is(ModTokenType::KEYWORD_LOGIC)) {
            advance();
            if (!consume(ModTokenType::PUNCTUATOR_COLON)) {
                addError("Expected ':' after 'logic'");
                return false;
            }
            
            if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
                addError("Expected '{' after 'logic:'");
                return false;
            }
            
            std::string logicCode;
            int braceCount = 1;
            
            while (!isAtEnd() && braceCount > 0) {
                const ModToken& tok = peek();
                
                if (tok.is(ModTokenType::PUNCTUATOR_LBRACE)) {
                    braceCount++;
                    logicCode += "{\n";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_RBRACE)) {
                    braceCount--;
                    if (braceCount > 0) {
                        logicCode += "\n}\n";
                    }
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_DOT_DOT_DOT)) {
                    logicCode += "...";
                    advance();
                } else if (tok.is(ModTokenType::IDENTIFIER) || 
                           tok.is(ModTokenType::NUMBER_LITERAL) ||
                           tok.is(ModTokenType::STRING_LITERAL) ||
                           tok.is(ModTokenType::KEYWORD_IF) ||
                           tok.is(ModTokenType::KEYWORD_ELSE) ||
                           tok.is(ModTokenType::KEYWORD_WHILE) ||
                           tok.is(ModTokenType::KEYWORD_PRINT) ||
                           tok.is(ModTokenType::KEYWORD_FN) ||
                           tok.is(ModTokenType::KEYWORD_ACTION)) {
                    logicCode += tok.text;
                    advance();
                    if (!peek().is(ModTokenType::PUNCTUATOR_LBRACE) && 
                        !peek().is(ModTokenType::PUNCTUATOR_RBRACE) &&
                        !peek().is(ModTokenType::PUNCTUATOR_DOT_DOT_DOT)) {
                        logicCode += " ";
                    }
                } else if (tok.is(ModTokenType::PUNCTUATOR_LESS)) {
                    logicCode += "<";
                    advance();
                    if (peek().is(ModTokenType::IDENTIFIER) || peek().is(ModTokenType::KEYWORD_ACTION)) {
                        logicCode += peek().text;
                        advance();
                        if (peek().is(ModTokenType::PUNCTUATOR_GREATER)) {
                            logicCode += ">";
                            advance();
                        }
                    }
                } else if (tok.is(ModTokenType::PUNCTUATOR_GREATER)) {
                    logicCode += ">";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_EQUAL)) {
                    logicCode += "=";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_EQUAL_EQUAL)) {
                    logicCode += "==";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_LESS_EQUAL)) {
                    logicCode += "<=";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_GREATER_EQUAL)) {
                    logicCode += ">=";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_LBRACKET)) {
                    logicCode += "[";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_RBRACKET)) {
                    logicCode += "]";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_LPAREN)) {
                    logicCode += "(";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_RPAREN)) {
                    logicCode += ")";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_PLUS)) {
                    logicCode += "+";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_MINUS)) {
                    logicCode += "-";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_STAR)) {
                    logicCode += "*";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_SLASH)) {
                    logicCode += "/";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_PERCENT)) {
                    logicCode += "%";
                    advance();
                } else if (tok.is(ModTokenType::PUNCTUATOR_COMMA)) {
                    logicCode += ",";
                    advance();
                } else {
                    advance();
                }
            }
            
            while (!logicCode.empty() && (logicCode.back() == ' ' || logicCode.back() == '\n')) {
                logicCode.pop_back();
            }
            
            syntax.logicCode = logicCode;
            
            if (g_debug_global) {
                std::cerr << "[mods debug] Parsed logic code:\n" << logicCode << std::endl;
            }
        } else {
            advance();
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close @add block");
        return false;
    }
    
    if (!syntax.name.empty()) {
        block.addedSyntaxes.push_back(syntax);
    }
    
    return true;
}

bool ModParser::parseModification(ModBlock& block) {
    if (!consume(ModTokenType::STRING_LITERAL)) {
        return false;
    }
    
    std::string from = tokens[current - 1].text;
    
    if (!consume(ModTokenType::PUNCTUATOR_ARROW)) {
        addError("Expected '=>' after source string");
        return false;
    }
    
    std::string to;
    if (consume(ModTokenType::STRING_LITERAL)) {
        to = tokens[current - 1].text;
    } else if (consume(ModTokenType::IDENTIFIER)) {
        to = tokens[current - 1].text;
    } else {
        addError("Expected string or identifier after '=>'");
        return false;
    }
    
    block.modifications.push_back(SyntaxModification(from, to, tokens[current - 1].line));
    return true;
}

bool ModsSystem::loadMod(const std::string& modName) {
    if (loadedModNames.find(modName) != loadedModNames.end()) {
        errors.push_back("Mod already loaded: " + modName);
        return false;
    }
    
    std::string modPath = "mods/" + modName + ".xfmod";
    
    std::filesystem::path filePath(modPath);
    if (!std::filesystem::exists(filePath)) {
        errors.push_back("Mod file not found: " + modPath);
        return false;
    }
    
    currentModName = modName;
    currentLoadOrder++;
    
    bool result = loadModFromFile(modPath);
    
    if (result) {
        loadedModNames.insert(modName);
    }
    
    return result;
}

bool ModsSystem::loadModFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        errors.push_back("Failed to open mod file: " + filePath);
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return loadModFromContent(content);
}

bool ModsSystem::loadModFromContent(const std::string& content) {
    ModLexer lexer(content);
    auto tokens = lexer.tokenize();
    
    if (lexer.hasErrors()) {
        for (const auto& err : lexer.getErrors()) {
            errors.push_back("Lexer error: " + err);
        }
        return false;
    }
    
    if (g_debug_global) {
        std::cerr << "[mods debug] Tokens from mod file:" << std::endl;
        for (const auto& tok : tokens) {
            std::cerr << "  " << tok.toString() << std::endl;
        }
    }
    
    ModParser parser(tokens);
    if (!parser.parse()) {
        for (const auto& err : parser.getErrors()) {
            errors.push_back("Parser error: " + err);
        }
        return false;
    }
    
    auto blocks = parser.getBlocks();
    for (auto& block : blocks) {
        for (auto& mod : block.modifications) {
            mod.modName = currentModName;
            mod.loadOrder = currentLoadOrder;
        }
        for (auto& syntax : block.addedSyntaxes) {
            syntax.modName = currentModName;
            syntax.loadOrder = currentLoadOrder;
        }
        loadedBlocks.push_back(std::move(block));
    }
    
    if (!checkForConflicts()) {
        return false;
    }
    
    collectModifications();
    registerSyntaxes();
    
    return true;
}

bool ModsSystem::checkForConflicts() {
    for (const auto& block : loadedBlocks) {
        for (const auto& mod : block.modifications) {
            if (!checkModificationConflict(mod)) {
                return false;
            }
        }
        
        for (const auto& syntax : block.addedSyntaxes) {
            if (!checkSyntaxConflict(syntax)) {
                return false;
            }
        }
    }
    return true;
}

bool ModsSystem::checkModificationConflict(const SyntaxModification& mod) {
    auto it = modificationRegistry.find(mod.from);
    if (it != modificationRegistry.end()) {
        const SyntaxModification& existing = it->second;
        
        conflicts.push_back(ConflictInfo(
            "modification",
            mod.from,
            existing.modName,
            existing.loadOrder,
            mod.modName,
            mod.loadOrder,
            mod.line
        ));
        
        errors.push_back("Modification conflict: '" + mod.from + "' is already modified by mod '" + 
                        existing.modName + "' (loaded first). Conflict with mod '" + mod.modName + 
                        "' at line " + std::to_string(mod.line));
        return false;
    }
    
    modificationRegistry[mod.from] = mod;
    return true;
}

bool ModsSystem::checkSyntaxConflict(const AddedSyntax& syntax) {
    auto it = syntaxRegistry.find(syntax.name);
    if (it != syntaxRegistry.end()) {
        const AddedSyntax& existing = it->second;
        
        conflicts.push_back(ConflictInfo(
            "syntax",
            syntax.name,
            existing.modName,
            existing.loadOrder,
            syntax.modName,
            syntax.loadOrder,
            syntax.line
        ));
        
        errors.push_back("Syntax conflict: '" + syntax.name + "' is already defined by mod '" + 
                        existing.modName + "' (loaded first). Conflict with mod '" + syntax.modName + 
                        "' at line " + std::to_string(syntax.line));
        return false;
    }
    
    return true;
}

void ModsSystem::collectModifications() {
    for (const auto& block : loadedBlocks) {
        for (const auto& mod : block.modifications) {
            allModifications.push_back(mod);
        }
        for (const auto& pubFunc : block.publicFunctions) {
            PublicFunction func = pubFunc;
            func.modName = block.name;
            allPublicFunctions.push_back(func);
        }
        for (const auto& internalName : block.internalFunctionNames) {
            allInternalFunctionNames.insert(internalName);
        }
    }
}

void ModsSystem::registerSyntaxes() {
    for (auto& block : loadedBlocks) {
        for (auto& syntax : block.addedSyntaxes) {
            std::string pattern = syntax.syntaxPattern;
            size_t pos = 0;
            
            while ((pos = pattern.find('<', pos)) != std::string::npos) {
                size_t endPos = pattern.find('>', pos);
                if (endPos != std::string::npos) {
                    std::string paramName = pattern.substr(pos + 1, endPos - pos - 1);
                    bool isAction = (paramName == "action");
                    syntax.parameters.push_back(SyntaxParameter(paramName, isAction));
                    if (!isAction) {
                        syntax.parameterOrder.push_back(paramName);
                    }
                    pos = endPos + 1;
                } else {
                    break;
                }
            }
            
            totalAddedSyntaxCount++;
            if (!validateSyntaxLimit()) {
                return;
            }
            
            allAddedSyntaxes.push_back(syntax);
            syntaxRegistry[syntax.name] = syntax;
        }
    }
}

std::string ModsSystem::applyModifications(const std::string& source) {
    std::string result = source;
    
    for (const auto& mod : allModifications) {
        size_t pos = 0;
        while ((pos = result.find(mod.to, pos)) != std::string::npos) {
            result.replace(pos, mod.to.length(), mod.from);
            pos += mod.from.length();
        }
    }
    
    return result;
}

std::string ModsSystem::expandSyntax(const std::string& source) {
    std::string result = source;
    
    for (const auto& syntax : allAddedSyntaxes) {
        std::string pattern = syntax.name;
        size_t pos = 0;
        
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            bool isWordBoundary = true;
            if (pos > 0) {
                char prevChar = result[pos - 1];
                if (isAlpha(prevChar) || isDigit(prevChar) || prevChar == '_') {
                    isWordBoundary = false;
                }
            }
            if (pos + pattern.length() < result.length()) {
                char nextChar = result[pos + pattern.length()];
                if (isAlpha(nextChar) || isDigit(nextChar) || nextChar == '_') {
                    isWordBoundary = false;
                }
            }
            
            if (!isWordBoundary) {
                pos++;
                continue;
            }
            
            size_t start = pos;
            pos += pattern.length();
            
            std::string usage;
            while (pos < result.length() && result[pos] != '{') {
                usage += result[pos];
                pos++;
            }
            
            while (!usage.empty() && (usage.front() == ' ' || usage.front() == '\t')) {
                usage.erase(0, 1);
            }
            while (!usage.empty() && (usage.back() == ' ' || usage.back() == '\t')) {
                usage.pop_back();
            }
            
            std::string matchedContent;
            if (pos < result.length() && result[pos] == '{') {
                int braceCount = 1;
                pos++;
                size_t contentStart = pos;
                while (pos < result.length() && braceCount > 0) {
                    if (result[pos] == '{') braceCount++;
                    else if (result[pos] == '}') braceCount--;
                    pos++;
                }
                matchedContent = result.substr(contentStart, pos - contentStart - 1);
            }
            
            auto args = parseSyntaxUsage(syntax, usage);
            std::string expanded = expandSingleSyntax(syntax, matchedContent, args);
            result.replace(start, pos - start, expanded);
            pos = start + expanded.length();
        }
    }
    
    return result;
}

std::unordered_map<std::string, std::string> ModsSystem::parseSyntaxUsage(const AddedSyntax& syntax, 
                                                                           const std::string& usage) {
    std::unordered_map<std::string, std::string> args;
    
    std::vector<std::string> usageTokens;
    std::string currentToken;
    
    for (size_t i = 0; i < usage.length(); i++) {
        char c = usage[i];
        if (c == '%') {
            if (!currentToken.empty()) {
                usageTokens.push_back(currentToken);
                currentToken.clear();
            }
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!currentToken.empty()) {
                usageTokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }
    if (!currentToken.empty()) {
        usageTokens.push_back(currentToken);
    }
    
    if (g_debug_global) {
        std::cerr << "[mods debug] parseSyntaxUsage:" << std::endl;
        std::cerr << "  Usage: '" << usage << "'" << std::endl;
        std::cerr << "  Tokens:" << std::endl;
        for (const auto& tok : usageTokens) {
            std::cerr << "    '" << tok << "'" << std::endl;
        }
        std::cerr << "  Parameter order:" << std::endl;
        for (const auto& param : syntax.parameterOrder) {
            std::cerr << "    " << param << std::endl;
        }
    }
    
    size_t paramIdx = 0;
    for (size_t i = 0; i < usageTokens.size() && paramIdx < syntax.parameterOrder.size(); i++) {
        args[syntax.parameterOrder[paramIdx]] = usageTokens[i];
        paramIdx++;
    }
    
    return args;
}

std::string ModsSystem::expandSingleSyntax(const AddedSyntax& syntax, 
                                            const std::string& matchedContent,
                                            const std::unordered_map<std::string, std::string>& args) {
    std::string result = syntax.logicCode;
    
    if (g_debug_global) {
        std::cerr << "[mods debug] expandSingleSyntax:" << std::endl;
        std::cerr << "  Logic code: " << result << std::endl;
        std::cerr << "  Args:" << std::endl;
        for (const auto& arg : args) {
            std::cerr << "    " << arg.first << " = " << arg.second << std::endl;
        }
        std::cerr << "  Action content: " << matchedContent << std::endl;
    }
    
    for (const auto& arg : args) {
        std::string placeholder = "<" + arg.first + ">";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), arg.second);
            pos += arg.second.length();
        }
    }
    
    size_t actionPos = result.find("<action>");
    if (actionPos != std::string::npos) {
        result.replace(actionPos, 8, matchedContent);
    }
    
    if (g_debug_global) {
        std::cerr << "  Expanded result: " << result << std::endl;
    }
    
    return result;
}

bool ModsSystem::validateSyntaxLimit() {
    if (totalAddedSyntaxCount > AddedSyntax::MAX_SYNTAX_LIMIT) {
        errors.push_back("Exceeded maximum syntax limit of " + 
                        std::to_string(AddedSyntax::MAX_SYNTAX_LIMIT) + 
                        " (current: " + std::to_string(totalAddedSyntaxCount) + ")");
        return false;
    }
    return true;
}

void ModsSystem::clear() {
    loadedBlocks.clear();
    allModifications.clear();
    allAddedSyntaxes.clear();
    errors.clear();
    conflicts.clear();
    syntaxRegistry.clear();
    modificationRegistry.clear();
    loadedModNames.clear();
    totalAddedSyntaxCount = 0;
    currentLoadOrder = 0;
    currentModName.clear();
}

}
