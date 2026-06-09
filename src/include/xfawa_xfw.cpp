#include "xfawa_xfw.h"
#include <fstream>
#include <sstream>

namespace xfawa {

// XfwParser implementation
XfwParser::XfwParser(const std::vector<ModToken>& toks) 
    : tokens(toks), current(0) {
}

bool XfwParser::parse() {
    // Basic implementation - just return true for now
    return true;
}

const ModToken& XfwParser::peek() const {
    static ModToken dummy;
    if (current >= tokens.size()) return dummy;
    return tokens[current];
}

const ModToken& XfwParser::peek(int offset) const {
    static ModToken dummy;
    if (current + offset >= tokens.size()) return dummy;
    return tokens[current + offset];
}

ModToken XfwParser::consume() {
    return tokens[current++];
}

bool XfwParser::consume(ModTokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

bool XfwParser::isAtEnd() const {
    return current >= tokens.size();
}

void XfwParser::advance() {
    if (!isAtEnd()) current++;
}

void XfwParser::addError(const std::string& message) {
    errors.push_back(message);
}

bool XfwParser::parseBlock() {
    return true;
}

bool XfwParser::parseMetaBlock() {
    return true;
}

bool XfwParser::parseBlockContent(XfwBlock& block) {
    return true;
}

bool XfwParser::parseFunction(XfwBlock& block) {
    return true;
}

bool XfwParser::parseExportedFunction(XfwExportedFunction& func) {
    return true;
}

std::string XfwParser::extractBlockContent() {
    return "";
}

// XfwSystem implementation
bool XfwSystem::loadLibrary(const std::string& libraryName) {
    std::string filePath = resolveLibraryPath(libraryName);
    return loadLibraryFromFile(filePath);
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
    file.close();
    
    return loadLibraryFromContent(content);
}

bool XfwSystem::loadLibraryFromContent(const std::string& content) {
    // Parse xfw library content
    XfwLexer lexer(content);
    std::vector<ModToken> tokens = lexer.tokenize();
    
    // Extract library name from #[meta] block
    std::regex libNameRegex("library:\\s*([\\w_]+)");
    std::smatch match;
    if (std::regex_search(content, match, libNameRegex)) {
        globalMeta.setLibraryName(match[1].str());
    }
    
    // Parse exported functions with namespace:fn pattern
    std::regex funcRegex("fn\\s+([\\w_]+):([\\w_]+)\\s*\\(([^)]*)\\)\\s*\\{");
    std::sregex_iterator it(content.begin(), content.end(), funcRegex);
    std::sregex_iterator end;
    
    while (it != end) {
        XfwExportedFunction func;
        func.ns = (*it)[1].str();
        func.name = (*it)[2].str();
        func.libraryName = globalMeta.libraryName;
        
        // Parse parameters
        std::string params = (*it)[3].str();
        if (!params.empty()) {
            std::regex paramRegex("([\\w_]+)");
            std::sregex_iterator pit(params.begin(), params.end(), paramRegex);
            std::sregex_iterator pend;
            while (pit != pend) {
                func.params.push_back((*pit)[1].str());
                ++pit;
            }
        }
        
        // Register function
        if (validateFunction(func)) {
            allExportedFunctions.push_back(func);
        }
        
        ++it;
    }
    
    // Register all functions
    registerFunctions();
    
    return !allExportedFunctions.empty();
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
    for (const auto& func : allExportedFunctions) {
        std::string fullName = func.ns + ":" + func.name;
        functionRegistry[fullName] = func;
    }
}

bool XfwSystem::validateFunction(const XfwExportedFunction& func) {
    if (func.name.empty()) {
        errors.push_back("Function name cannot be empty");
        return false;
    }
    return true;
}

std::string XfwSystem::resolveLibraryPath(const std::string& libraryName) {
    // Try current directory first
    if (std::filesystem::exists(libraryName)) {
        return libraryName;
    }
    
    // Try libs directory
    std::string libsPath = "libs/" + libraryName;
    if (std::filesystem::exists(libsPath)) {
        return libsPath;
    }
    
    // Return as-is if not found
    return libraryName;
}

}
