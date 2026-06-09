#ifndef XFAWA_XFW_H
#define XFAWA_XFW_H

#include "xfawa_error.h"
#include "xfawa_mods_system.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>

namespace xfawa {

// xfw library file information
struct XfwMetaInfo {
    std::string author;
    std::string version;
    std::string description;
    std::string license;
    std::string libraryName;  // Library name
    int line;
    bool hasMeta;
    
    XfwMetaInfo() : line(0), hasMeta(false) {}
    
    void setAuthor(const std::string& a) { author = a; hasMeta = true; }
    void setVersion(const std::string& v) { version = v; hasMeta = true; }
    void setDescription(const std::string& d) { description = d; hasMeta = true; }
    void setLicense(const std::string& l) { license = l; hasMeta = true; }
    void setLibraryName(const std::string& n) { libraryName = n; hasMeta = true; }
};

// xfw exported function (inherits from xfmod's open function mechanism)
struct XfwExportedFunction {
    std::string ns;           // Namespace
    std::string name;         // Function name
    std::vector<std::string> params;  // Parameter list
    std::string body;         // Function body
    std::string signature;    // Function signature (for type checking)
    int line;
    std::string libraryName;  // Library name
    
    XfwExportedFunction() : line(0) {}
};

// xfw library block
struct XfwBlock {
    std::string name;
    std::vector<XfwExportedFunction> exportedFunctions;
    std::unordered_set<std::string> internalFunctionNames;
    std::string code;  // Complete code content
    
    XfwBlock(const std::string& n) : name(n) {}
};

// xfw lexer (inherits from ModLexer)
class XfwLexer : public ModLexer {
public:
    explicit XfwLexer(const std::string& source) : ModLexer(source) {}
};

// xfw parser
class XfwParser {
private:
    std::vector<ModToken> tokens;
    size_t current;
    std::vector<std::string> errors;
    std::vector<XfwBlock> blocks;
    XfwMetaInfo metaInfo;
    
public:
    explicit XfwParser(const std::vector<ModToken>& toks);
    
    bool parse();
    const std::vector<XfwBlock>& getBlocks() const { return blocks; }
    const XfwMetaInfo& getMetaInfo() const { return metaInfo; }
    
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    
private:
    const ModToken& peek() const;
    const ModToken& peek(int offset) const;
    ModToken consume();
    bool consume(ModTokenType type);
    bool isAtEnd() const;
    void advance();
    void addError(const std::string& message);
    
    bool parseBlock();
    bool parseMetaBlock();
    bool parseBlockContent(XfwBlock& block);
    bool parseFunction(XfwBlock& block);
    bool parseExportedFunction(XfwExportedFunction& func);
    
    std::string extractBlockContent();
};

// xfw library system
class XfwSystem {
private:
    std::vector<XfwBlock> loadedBlocks;
    std::vector<XfwExportedFunction> allExportedFunctions;
    std::unordered_set<std::string> allInternalFunctionNames;
    std::vector<std::string> errors;
    std::unordered_map<std::string, XfwExportedFunction> functionRegistry;  // Function registry
    std::unordered_set<std::string> loadedLibraryNames;  // Loaded library names
    XfwMetaInfo globalMeta;
    
public:
    XfwSystem() {}
    
    // Load xfw library file
    bool loadLibrary(const std::string& libraryName);
    bool loadLibraryFromFile(const std::string& filePath);
    bool loadLibraryFromContent(const std::string& content);
    
    // Get exported functions
    const XfwExportedFunction* getFunction(const std::string& ns, const std::string& name) const;
    const XfwExportedFunction* getFunction(const std::string& fullName) const;
    
    // Check if function exists
    bool hasFunction(const std::string& ns, const std::string& name) const;
    bool hasFunction(const std::string& fullName) const;
    
    // Get all exported functions
    const std::vector<XfwExportedFunction>& getExportedFunctions() const { return allExportedFunctions; }
    
    // Get meta information
    const XfwMetaInfo& getMetaInfo() const { return globalMeta; }
    bool hasMetaInfo() const { return globalMeta.hasMeta; }
    
    // Error handling
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    
    // Clear
    void clear();
    
    // Check if loaded
    bool isLoaded(const std::string& libraryName) const;
    
private:
    void registerFunctions();
    bool validateFunction(const XfwExportedFunction& func);
    std::string resolveLibraryPath(const std::string& libraryName);
};

}

#endif
