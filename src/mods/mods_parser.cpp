#include "xfawa_mods_parser.h"
#include <sstream>
#include <algorithm>

namespace xfawa {

ModsParser::ModsParser(const std::vector<ModsToken>& toks) : tokens(toks), current(0) {}

const ModsToken& ModsParser::peek() const {
    if (current < tokens.size()) {
        return tokens[current];
    }
    return tokens.back();
}

const ModsToken& ModsParser::peek(int offset) const {
    size_t pos = current + offset;
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    return tokens.back();
}

ModsToken ModsParser::consume() {
    if (current < tokens.size()) {
        return tokens[current++];
    }
    return tokens.back();
}

bool ModsParser::consume(ModsTokenType type) {
    if (peek().is(type)) {
        current++;
        return true;
    }
    return false;
}

bool ModsParser::isAtEnd() const {
    return peek().is(ModsTokenType::END_OF_FILE);
}

void ModsParser::advance() {
    if (current < tokens.size()) {
        current++;
    }
}

void ModsParser::addError(const std::string& message) {
    errors.push_back(message + " at line " + std::to_string(peek().line));
}

}