#include "include/xfawa_xfw.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace xfawa {

// XfwParser 实现

XfwParser::XfwParser(const std::vector<ModToken>& toks) 
    : tokens(toks), current(0) {}

const ModToken& XfwParser::peek() const {
    return tokens[current];
}

const ModToken& XfwParser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos >= tokens.size()) {
        static ModToken eof(ModTokenType::END_OF_FILE, "", -1, -1);
        return eof;
    }
    return tokens[pos];
}

ModToken XfwParser::consume() {
    return tokens[current++];
}

bool XfwParser::consume(ModTokenType type) {
    if (peek().is(type)) {
        advance();
        return true;
    }
    return false;
}

bool XfwParser::isAtEnd() const {
    return peek().is(ModTokenType::END_OF_FILE);
}

void XfwParser::advance() {
    if (!isAtEnd()) {
        current++;
    }
}

void XfwParser::addError(const std::string& message) {
    errors.push_back("Line " + std::to_string(peek().line) + ": " + message);
}

bool XfwParser::parse() {
    while (!isAtEnd()) {
        if (peek().is(ModTokenType::PUNCTUATOR_HASH)) {
            advance();
            
            // 检查是否是元信息块 #[meta]
            if (peek().is(ModTokenType::IDENTIFIER) && peek().text == "meta") {
                advance();
                if (!parseMetaBlock()) {
                    return false;
                }
            } else if (peek().is(ModTokenType::IDENTIFIER)) {
                // 普通库块
                std::string blockName = peek().text;
                advance();
                
                if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
                    addError("Expected '{' after block name");
                    return false;
                }
                
                XfwBlock block(blockName);
                if (!parseBlockContent(block)) {
                    return false;
                }
                
                blocks.push_back(block);
            } else {
                addError("Expected block name or 'meta' after '#'");
                return false;
            }
        } else {
            advance();
        }
    }
    
    return true;
}

bool XfwParser::parseMetaBlock() {
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' after 'meta'");
        return false;
    }
    
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
        if (peek().is(ModTokenType::IDENTIFIER)) {
            std::string key = peek().text;
            advance();
            
            if (!consume(ModTokenType::PUNCTUATOR_COLON)) {
                addError("Expected ':' after meta key");
                return false;
            }
            
            std::string value;
            if (peek().is(ModTokenType::STRING_LITERAL)) {
                value = peek().text;
                // 去除引号
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                advance();
            } else if (peek().is(ModTokenType::NUMBER_LITERAL)) {
                value = peek().text;
                advance();
            } else if (peek().is(ModTokenType::IDENTIFIER)) {
                value = peek().text;
                advance();
            } else {
                addError("Expected value after ':'");
                return false;
            }
            
            if (key == "author") {
                metaInfo.setAuthor(value);
            } else if (key == "version") {
                metaInfo.setVersion(value);
            } else if (key == "description") {
                metaInfo.setDescription(value);
            } else if (key == "license") {
                metaInfo.setLicense(value);
            } else if (key == "library") {
                metaInfo.setLibraryName(value);
            }
        } else {
            advance();
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close meta block");
        return false;
    }
    
    return true;
}

bool XfwParser::parseBlockContent(XfwBlock& block) {
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
        if (peek().is(ModTokenType::KEYWORD_FN)) {
            if (!parseFunction(block)) {
                return false;
            }
        } else {
            advance();
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RBRACE)) {
        addError("Expected '}' to close block");
        return false;
    }
    
    return true;
}

bool XfwParser::parseFunction(XfwBlock& block) {
    if (!consume(ModTokenType::KEYWORD_FN)) {
        return false;
    }
    
    // 解析函数名（可能包含命名空间）
    std::string functionName;
    std::string ns;
    
    if (peek().is(ModTokenType::IDENTIFIER)) {
        functionName = peek().text;
        advance();
        
        // 检查是否有命名空间（ns:name格式）
        if (peek().is(ModTokenType::PUNCTUATOR_COLON)) {
            advance();
            ns = functionName;
            if (peek().is(ModTokenType::IDENTIFIER)) {
                functionName = peek().text;
                advance();
            } else {
                addError("Expected function name after namespace");
                return false;
            }
        }
    } else {
        addError("Expected function name");
        return false;
    }
    
    // 解析参数列表
    if (!consume(ModTokenType::PUNCTUATOR_LPAREN)) {
        addError("Expected '(' after function name");
        return false;
    }
    
    std::vector<std::string> params;
    while (!isAtEnd() && !peek().is(ModTokenType::PUNCTUATOR_RPAREN)) {
        if (peek().is(ModTokenType::IDENTIFIER)) {
            params.push_back(peek().text);
            advance();
            
            if (peek().is(ModTokenType::PUNCTUATOR_COMMA)) {
                advance();
            }
        } else {
            advance();
        }
    }
    
    if (!consume(ModTokenType::PUNCTUATOR_RPAREN)) {
        addError("Expected ')' after parameters");
        return false;
    }
    
    // 解析函数体
    if (!consume(ModTokenType::PUNCTUATOR_LBRACE)) {
        addError("Expected '{' to start function body");
        return false;
    }
    
    std::string body = extractBlockContent();
    
    // 创建导出函数
    XfwExportedFunction func;
    func.ns = ns;
    func.name = functionName;
    func.params = params;
    func.body = body;
    func.line = peek().line;
    func.libraryName = block.name;
    
    if (!ns.empty()) {
        // 有命名空间的函数是导出函数
        block.exportedFunctions.push_back(func);
    } else {
        // 无命名空间的函数是内部函数
        block.internalFunctionNames.insert(functionName);
    }
    
    return true;
}

std::string XfwParser::extractBlockContent() {
    std::string content;
    int braceCount = 1;
    
    while (!isAtEnd() && braceCount > 0) {
        if (peek().is(ModTokenType::PUNCTUATOR_LBRACE)) {
            braceCount++;
            content += "{ ";
        } else if (peek().is(ModTokenType::PUNCTUATOR_RBRACE)) {
            braceCount--;
            if (braceCount > 0) {
                content += "} ";
            }
        } else if (peek().is(ModTokenType::PUNCTUATOR_SEMICOLON)) {
            content += "; ";
        } else {
            content += peek().text + " ";
        }
        advance();
    }
    
    return content;
}

// XfwSystem 实现

bool XfwSystem::loadLibrary(const std::string& libraryName) {
    std::string path = resolveLibraryPath(libraryName);
    return loadLibraryFromFile(path);
}

bool XfwSystem::loadLibraryFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        errors.push_back("Cannot open xfw library file: " + filePath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    return loadLibraryFromContent(content);
}

bool XfwSystem::loadLibraryFromContent(const std::string& content) {
    XfwLexer lexer(content);
    auto tokens = lexer.tokenize();
    
    if (lexer.hasErrors()) {
        const auto& lexerErrors = lexer.getErrors();
        errors.insert(errors.end(), lexerErrors.begin(), lexerErrors.end());
        return false;
    }
    
    XfwParser parser(tokens);
    if (!parser.parse()) {
        const auto& parserErrors = parser.getErrors();
        errors.insert(errors.end(), parserErrors.begin(), parserErrors.end());
        return false;
    }
    
    const auto& parsedBlocks = parser.getBlocks();
    loadedBlocks.insert(loadedBlocks.end(), parsedBlocks.begin(), parsedBlocks.end());
    
    // 更新元信息
    if (parser.hasMetaInfo()) {
        globalMeta = parser.getMetaInfo();
    }
    
    // 注册函数
    registerFunctions();
    
    return true;
}

const XfwExportedFunction* XfwSystem::getFunction(const std::string& ns, const std::string& name) const {
    std::string fullName = ns + ":" + name;
    return getFunction(fullName);
}

const XfwExportedFunction* XfwSystem::getFunction(const std::string& fullName) const {
    auto it = functionRegistry.find(fullName);
    if (it != functionRegistry.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool XfwSystem::hasFunction(const std::string& ns, const std::string& name) const {
    return getFunction(ns, name) != nullptr;
}

bool XfwSystem::hasFunction(const std::string& fullName) const {
    return getFunction(fullName) != nullptr;
}

void XfwSystem::clear() {
    loadedBlocks.clear();
    allExportedFunctions.clear();
    allInternalFunctionNames.clear();
    errors.clear();
    functionRegistry.clear();
    loadedLibraryNames.clear();
    globalMeta = XfwMetaInfo();
}

bool XfwSystem::isLoaded(const std::string& libraryName) const {
    return loadedLibraryNames.find(libraryName) != loadedLibraryNames.end();
}

void XfwSystem::registerFunctions() {
    for (const auto& block : loadedBlocks) {
        for (const auto& func : block.exportedFunctions) {
            std::string fullName = func.ns + ":" + func.name;
            
            // 检查函数是否已注册
            if (functionRegistry.find(fullName) != functionRegistry.end()) {
                errors.push_back("Function '" + fullName + "' is already registered");
                continue;
            }
            
            // 验证函数
            if (!validateFunction(func)) {
                continue;
            }
            
            // 注册函数
            functionRegistry[fullName] = func;
            allExportedFunctions.push_back(func);
        }
        
        // 注册内部函数名
        for (const auto& internalName : block.internalFunctionNames) {
            allInternalFunctionNames.insert(internalName);
        }
        
        // 记录已加载的库名
        loadedLibraryNames.insert(block.name);
    }
}

bool XfwSystem::validateFunction(const XfwExportedFunction& func) {
    // 检查命名空间是否为保留命名空间
    static const std::unordered_set<std::string> reservedNamespaces = {
        "core", "sys", "runtime", "compiler", "internal", "builtin", "std", "ext", "lib"
    };
    
    if (!func.ns.empty() && reservedNamespaces.find(func.ns) != reservedNamespaces.end()) {
        errors.push_back("Cannot use reserved namespace '" + func.ns + "' for function '" + func.name + "'");
        return false;
    }
    
    // 检查是否包含多级命名空间
    if (!func.ns.empty() && func.ns.find(':') != std::string::npos) {
        errors.push_back("Multi-level namespaces are not allowed: " + func.ns);
        return false;
    }
    
    return true;
}

std::string XfwSystem::resolveLibraryPath(const std::string& libraryName) {
    // 尝试添加.xfw扩展名
    std::string path = libraryName;
    if (path.size() < 4 || path.substr(path.size() - 4) != ".xfw") {
        path += ".xfw";
    }
    
    // 检查文件是否存在
    if (std::filesystem::exists(path)) {
        return path;
    }
    
    // 尝试在libs目录中查找
    std::string libsPath = "libs/" + path;
    if (std::filesystem::exists(libsPath)) {
        return libsPath;
    }
    
    return path;
}

}
