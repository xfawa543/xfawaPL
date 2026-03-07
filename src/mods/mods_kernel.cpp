#include "xfawa_mods_kernel.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <filesystem>

namespace xfawa {

ModsKernel::ModsKernel() {}

void ModsKernel::loadModFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open mod file: " << filepath << std::endl;
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    parseModContent(content);
}

void ModsKernel::parseModContent(const std::string& content) {
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#') continue;
        
        if (line.find("=>") != std::string::npos) {
            parseModification(line);
        } else if (line.find("syntax:") != std::string::npos) {
            parseAddSyntax(line);
        }
    }
}

void ModsKernel::parseModification(const std::string& line) {
    size_t arrow_pos = line.find("=>");
    if (arrow_pos == std::string::npos) return;
    
    std::string from = trim(line.substr(0, arrow_pos));
    std::string to = trim(line.substr(arrow_pos + 2));
    
    if (!from.empty() && !to.empty()) {
        modifications[from] = to;
    }
}

void ModsKernel::parseAddSyntax(const std::string& line) {
    size_t syntax_pos = line.find("syntax:");
    if (syntax_pos == std::string::npos) return;
    
    std::string syntax_part = trim(line.substr(syntax_pos + 7));
    
    size_t name_pos = syntax_part.find(' ');
    if (name_pos == std::string::npos) return;
    
    std::string syntax_name = trim(syntax_part.substr(0, name_pos));
    std::string syntax_def = trim(syntax_part.substr(name_pos + 1));
    
    if (!syntax_name.empty() && !syntax_def.empty()) {
        added_syntax.push_back({syntax_name, syntax_def});
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

std::vector<std::string> ModsKernel::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream iss(s);
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string ModsKernel::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

}