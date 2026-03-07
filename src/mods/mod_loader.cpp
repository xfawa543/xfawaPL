#include "mod_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace xfawa {

ModLoader::ModLoader() {
}

bool ModLoader::loadMod(const std::string& modName) {
    std::string modPath = "mods/" + modName + ".xfmod";
    
    std::filesystem::path filePath(modPath);
    if (!std::filesystem::exists(filePath)) {
        addError("Mod file not found: " + modPath);
        return false;
    }
    
    return loadModFromFile(modPath);
}

bool ModLoader::loadModFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        addError("Failed to open mod file: " + filePath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    return parseModFile(content);
}

std::string ModLoader::applyModifications(const std::string& source) {
    std::string result = source;
    
    for (const auto& mod : syntaxModifications) {
        size_t pos = 0;
        while ((pos = result.find(mod.from, pos)) != std::string::npos) {
            result.replace(pos, mod.from.length(), mod.to);
            pos += mod.to.length();
        }
    }
    
    return result;
}

bool ModLoader::parseModFile(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        std::string trimmedLine = line;
        size_t start = trimmedLine.find_first_not_of(" \t");
        if (start != std::string::npos) {
            trimmedLine = trimmedLine.substr(start);
        }
        size_t end = trimmedLine.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            trimmedLine = trimmedLine.substr(0, end + 1);
        }
        
        if (trimmedLine.empty()) continue;
        
        if (trimmedLine[0] == '"') {
            size_t endPos = trimmedLine.find('"', 1);
            if (endPos != std::string::npos) {
                std::string from = trimmedLine.substr(1, endPos - 1);
                size_t arrowPos = trimmedLine.find("=>", endPos + 1);
                if (arrowPos != std::string::npos) {
                    size_t startTo = arrowPos + 2;
                    while (startTo < trimmedLine.length() && (trimmedLine[startTo] == ' ' || trimmedLine[startTo] == '\t')) {
                        startTo++;
                    }
                    size_t endTo = trimmedLine.find('"', startTo);
                    if (endTo != std::string::npos) {
                        std::string to = trimmedLine.substr(startTo, endTo - startTo);
                        syntaxModifications.push_back({from, to});
                    }
                }
            }
        }
    }
    
    return true;
}

bool ModLoader::parseSyntaxModification(const std::string& line) {
    return true;
}

bool ModLoader::parseNewSyntax(const std::string& blockName, const std::string& blockContent) {
    return true;
}

std::string ModLoader::extractBlockContent(const std::string& content, const std::string& blockName) {
    return "";
}

void ModLoader::addError(const std::string& message) {
    errors.push_back(message);
}

}