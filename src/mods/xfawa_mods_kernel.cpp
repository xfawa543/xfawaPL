#include "xfawa_mods_kernel.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace xfawa {

ModsKernel::ModsKernel() {
}

void ModsKernel::loadModFile(const std::string& filepath) {
    std::filesystem::path filePath(filepath);
    if (!std::filesystem::exists(filePath)) {
        return;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    parseModContent(content);
}

void ModsKernel::parseModContent(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        std::string trimmedLine = trim(line);
        
        if (trimmedLine.empty()) continue;
        if (trimmedLine[0] == '#') {
            continue;
        }
        if (trimmedLine[0] == '"') {
            parseModification(trimmedLine);
        } else if (trimmedLine.find("@add") == 0) {
            parseAddSyntax(trimmedLine);
        }
    }
}

void ModsKernel::applyModifications(std::string& source) {
    for (const auto& mod : modifications) {
        size_t pos = 0;
        while ((pos = source.find(mod.first, pos)) != std::string::npos) {
            source.replace(pos, mod.first.length(), mod.second);
            pos += mod.second.length();
        }
    }
}

bool ModsKernel::hasModifications() const {
    return !modifications.empty() || !added_syntax.empty();
}

void ModsKernel::clear() {
    modifications.clear();
    added_syntax.clear();
}

void ModsKernel::parseModification(const std::string& line) {
    size_t firstQuote = line.find('"');
    size_t secondQuote = line.find('"', firstQuote + 1);
    
    if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
        std::string from = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        
        size_t arrowPos = line.find("=>", secondQuote + 1);
        if (arrowPos != std::string::npos) {
            size_t thirdQuote = line.find('"', arrowPos + 2);
            if (thirdQuote != std::string::npos) {
                std::string to = line.substr(thirdQuote + 1);
                modifications[from] = to;
            } else {
                errors.push_back("Invalid modification syntax: " + line);
            }
        } else {
            errors.push_back("Invalid modification syntax: " + line);
        }
    } else {
        errors.push_back("Invalid modification syntax: " + line);
    }
}

void ModsKernel::parseAddSyntax(const std::string& line) {
    std::string rest = line.substr(4);
    size_t bracePos = rest.find('{');
    
    if (bracePos != std::string::npos) {
        std::string syntaxPart = rest.substr(0, bracePos);
        std::string syntaxStr = trim(syntaxPart);
        
        size_t endBrace = rest.find_last_of('}');
        if (endBrace != std::string::npos) {
            std::string content = rest.substr(bracePos + 1, endBrace - bracePos - 1);
            added_syntax.push_back({syntaxStr, content});
        }
    }
}

std::vector<std::string> ModsKernel::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string ModsKernel::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) {
        return s.substr(start);
    }
    
    return s.substr(start, end - start + 1);
}

}