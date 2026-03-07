#ifndef XFAWA_MOD_LOADER_H
#define XFAWA_MOD_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace xfawa {

struct SyntaxModification {
    std::string from;
    std::string to;
};

struct NewSyntax {
    std::string name;
    std::string syntax;
    std::string logic;
    std::string action;
};

class ModLoader {
public:
    ModLoader();
    
    bool loadMod(const std::string& modName);
    bool loadModFromFile(const std::string& filePath);
    
    std::string applyModifications(const std::string& source);
    
    const std::vector<NewSyntax>& getNewSyntaxes() const { return newSyntaxes; }
    const std::vector<SyntaxModification>& getSyntaxModifications() const { return syntaxModifications; }
    
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    
private:
    std::vector<SyntaxModification> syntaxModifications;
    std::vector<NewSyntax> newSyntaxes;
    std::vector<std::string> errors;
    
    bool parseModFile(const std::string& content);
    bool parseSyntaxModification(const std::string& line);
    bool parseNewSyntax(const std::string& blockName, const std::string& blockContent);
    std::string extractBlockContent(const std::string& content, const std::string& blockName);
    void addError(const std::string& message);
};

}

#endif