#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b-a);
}

static std::string escape_literal(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += (char)c;
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <input.xf> [-o output_exe]\n", argv[0]);
        return 1;
    }
    const char* infile = argv[1];
    const char* outfile = nullptr;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "-o") == 0 && i+1 < argc) { outfile = argv[++i]; }
    }
    std::ifstream ifs(infile, std::ios::binary);
    if (!ifs) { std::fprintf(stderr, "Cannot open input file %s\n", infile); return 2; }
    std::ostringstream ss; ss << ifs.rdbuf(); std::string code = ss.str();
    size_t ob = code.find('{');
    if (ob == std::string::npos) { std::fprintf(stderr, "No top-level block found in %s\n", infile); return 3; }
    size_t idx = ob + 1; int lvl = 1;
    size_t start = idx;
    while (idx < code.size() && lvl > 0) {
        if (code[idx] == '{') lvl++; else if (code[idx] == '}') lvl--; idx++;
    }
    if (lvl != 0) { std::fprintf(stderr, "Mismatched braces in %s\n", infile); return 4; }
    size_t end = idx - 1;
    std::string block = code.substr(start, end - start);
    std::istringstream lines(block);
    std::string line;
    std::vector<std::string> prints;
    while (std::getline(lines, line)) {
        std::string t = trim(line);
        if (t.empty()) continue;
        if (t.rfind("print(\"", 0) == 0) {
            size_t p = 7; // after print(" ")
            std::string accum;
            bool escaping = false;
            for (; p < t.size(); ++p) {
                char c = t[p];
                if (!escaping && c == '\\') { escaping = true; continue; }
                if (!escaping && c == '"') break;
                if (escaping) {
                    if (c == 'n') accum += '\n';
                    else if (c == 't') accum += '\t';
                    else accum += c;
                    escaping = false;
                } else accum += c;
            }
            prints.push_back(accum);
        }
    }
    if (prints.empty()) { std::fprintf(stderr, "No print() statements found in block; nothing to emit.\n"); return 5; }
    // generate temporary C++ source
    std::srand((unsigned int)std::time(NULL));
    int r = std::rand() & 0xffff;
    char tmpname[256]; std::snprintf(tmpname, sizeof(tmpname), "temp_llvm_%d_%d.cpp", (int)r, (int)time(NULL));
    std::ofstream out(tmpname, std::ios::binary);
    if (!out) { std::fprintf(stderr, "Cannot create temp file %s\n", tmpname); return 6; }
    out << "#include <iostream>\n";
    out << "#ifdef _WIN32\n#include <windows.h>\n#endif\n";
    out << "int main() {\n";
    out << "#ifdef _WIN32\n    SetConsoleOutputCP(CP_UTF8);\n    SetConsoleCP(CP_UTF8);\n#endif\n";
    for (size_t i = 0; i < prints.size(); ++i) {
        std::string lit = escape_literal(prints[i]);
        out << "    std::cout << u8\"" << lit << "\" << std::endl;\n";
    }
    out << "    return 0;\n}\n";
    out.close();
    // choose compiler: try clang++, fallback to g++
    const char* comp = "clang++";
    std::string cmd;
    if (!outfile) {
        std::string base(infile);
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        base += "_llvm.exe";
        outfile = strdup(base.c_str());
    }
    // try clang++ first
    cmd = std::string(comp) + " -std=c++17 -O2 \"" + tmpname + "\" -o \"" + outfile + "\" 2>&1";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        comp = "g++";
        cmd = std::string(comp) + " -std=c++17 -O2 \"" + tmpname + "\" -o \"" + outfile + "\" 2>&1";
        rc = std::system(cmd.c_str());
    }
    if (rc != 0) {
        std::fprintf(stderr, "Compilation failed (tried clang++ and g++). Command: %s\n", cmd.c_str());
        std::fprintf(stderr, "Temp source: %s\n", tmpname);
        return 7;
    }
    std::printf("LLVM-backend prototype: generated %s from %s\n", outfile, infile);
    std::printf("Temp source: %s\n", tmpname);
    return 0;
}
