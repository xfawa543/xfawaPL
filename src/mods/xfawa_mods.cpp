#include "xfawa_mods.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace xfawa {

ModsSystem::ModsSystem() {
}

bool ModsSystem::loadMod(const std::string& modName, const std::string& modPath) {
    std::ifstream file(modPath);
    if (!file.is_open()) {
        return false;
    }
    
    auto mod = std::make_unique<ModFile>(modName);
    std::string line;
    int lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        line = std::regex_replace(line, std::regex("//.*"), "");
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        if (line.find("macro") == 0) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string from = line.substr(6, eqPos - 6);
                std::string to = line.substr(eqPos + 1);
                from.erase(0, from.find_first_not_of(" \t\""));
                from.erase(from.find_last_not_of(" \t\"") + 1);
                to.erase(0, to.find_first_not_of(" \t\""));
                to.erase(to.find_last_not_of(" \t\"") + 1);
                
                SourceLocation loc;
                loc.line = lineNum;
                mod->addMacroReplacement(MacroReplacement(from, to, loc));
            }
        } else if (line.find("syntax") == 0) {
            size_t parenPos = line.find('(');
            if (parenPos != std::string::npos) {
                std::string name = line.substr(7, parenPos - 7);
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                
                size_t closeParen = line.find(')', parenPos);
                std::string argsPattern = line.substr(parenPos + 1, closeParen - parenPos - 1);
                
                SourceLocation loc;
                loc.line = lineNum;
                
                auto handler = [argsPattern](const std::string& args) -> std::string {
                    return argsPattern;
                };
                
                mod->addSyntaxExtension(SyntaxExtension(name, argsPattern, handler, loc));
            }
        }
    }
    
    loadedMods[modName] = std::move(mod);
    
    for (const auto& macro : mod->macroReplacements) {
        allMacroReplacements.push_back(macro);
    }
    
    for (const auto& ext : mod->syntaxExtensions) {
        allSyntaxExtensions.push_back(ext);
    }
    
    return true;
}

bool ModsSystem::applyMacroReplacements(std::string& source) {
    bool modified = false;
    
    for (const auto& macro : allMacroReplacements) {
        size_t pos = 0;
        while ((pos = source.find(macro.from, pos)) != std::string::npos) {
            source.replace(pos, macro.from.length(), macro.to);
            pos += macro.to.length();
            modified = true;
        }
    }
    
    return modified;
}

bool ModsSystem::hasSyntaxExtension(const std::string& name) const {
    for (const auto& ext : allSyntaxExtensions) {
        if (ext.name == name) {
            return true;
        }
    }
    return false;
}

std::string ModsSystem::applySyntaxExtension(const std::string& name, const std::string& args) {
    for (const auto& ext : allSyntaxExtensions) {
        if (ext.name == name) {
            return ext.handler(args);
        }
    }
    return args;
}

}