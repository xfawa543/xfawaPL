#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b-a);
}

static std::string escape_ir(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '\\') out += "\\5C";
        else if (c == '"') out += "\\22";
        else if (c == '\n') out += "\\0A";
        else if (c == '\r') out += "\\0D";
        else if (c == '\t') out += "\\09";
        else if (c < 0x20 || c == 0x7f) {
            char buf[8]; snprintf(buf, sizeof(buf), "\\%02X", c); out += buf;
        } else {
            out.push_back((char)c);
        }
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "Usage: %s <input.xf> [-o output_exe]\n", argv[0]); return 1; }
    const char* infile = argv[1]; const char* outfile = nullptr;
    for (int i = 2; i < argc; ++i) if (std::strcmp(argv[i], "-o") == 0 && i+1 < argc) outfile = argv[++i];
    std::ifstream ifs(infile, std::ios::binary);
    if (!ifs) { std::fprintf(stderr, "Cannot open %s\n", infile); return 2; }
    std::ostringstream ss; ss << ifs.rdbuf(); std::string code = ss.str();
    size_t ob = code.find('{'); if (ob == std::string::npos) { std::fprintf(stderr, "No block\n"); return 3; }
    size_t idx = ob+1; int lvl = 1; while (idx < code.size() && lvl > 0) { if (code[idx]=='{') lvl++; else if (code[idx]=='}') lvl--; idx++; }
    if (lvl != 0) { std::fprintf(stderr, "Mismatched braces\n"); return 4; }
    size_t start = ob+1; size_t end = idx-1; std::string block = code.substr(start, end-start);
    std::istringstream lines(block);
    std::string line; std::vector<std::string> prints;
    while (std::getline(lines, line)) {
        std::string t = trim(line);
        if (t.empty()) continue;
        if (t.rfind("print(\"", 0) == 0) {
            size_t p = 7; std::string accum; bool esc = false;
            for (; p < t.size(); ++p) {
                char c = t[p]; if (!esc && c=='\\') { esc = true; continue; } if (!esc && c=='"') break; if (esc) { if (c=='n') accum+='\n'; else if (c=='t') accum+='\t'; else accum+=c; esc=false; } else accum+=c; }
            prints.push_back(accum);
        }
    }
    if (prints.empty()) { std::fprintf(stderr, "No print() found\n"); return 5; }
    char tmpll[256]; std::snprintf(tmpll, sizeof(tmpll), "temp_llvm_ir_%d.ll", (int)time(NULL));
    std::ofstream out(tmpll, std::ios::binary);
    if (!out) { std::fprintf(stderr, "Cannot create %s\n", tmpll); return 6; }
    out << "; ModuleID = 'xfawac_llvm_ir'\n";
    out << "declare i32 @puts(i8*)\n";
    out << "declare i32 @SetConsoleOutputCP(i32)\n";
    out << "@.null = private unnamed_addr constant [1 x i8] zeroinitializer\n\n";
    for (size_t i = 0; i < prints.size(); ++i) {
        std::string lit = prints[i]; std::string esc = escape_ir(lit);
        out << "@.str" << i << " = private unnamed_addr constant [" << (esc.size()+1) << " x i8] c\"" << esc << "\\00\", align 1\n";
    }
    out << "\ndefine i32 @main() {\nentry:\n";
    out << "  call i32 @SetConsoleOutputCP(i32 65001)\n";
    for (size_t i = 0; i < prints.size(); ++i) {
        out << "  %ptr" << i << " = getelementptr inbounds [" << (escape_ir(prints[i]).size()+1) << " x i8], [" << (escape_ir(prints[i]).size()+1) << " x i8]* @.str" << i << ", i32 0, i32 0\n";
        out << "  call i32 @puts(i8* %ptr" << i << ")\n";
    }
    out << "  ret i32 0\n}\n";
    out.close();
    if (!outfile) { std::string base(infile); size_t d = base.rfind('.'); if (d!=std::string::npos) base = base.substr(0,d); base += "_ir.exe"; outfile = strdup(base.c_str()); }
    std::string cmd = std::string("clang ") + tmpll + " -o " + outfile + " 2>&1";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::fprintf(stderr, "clang failed to compile IR. Command: %s\n", cmd.c_str()); std::fprintf(stderr, "Temp IR: %s\n", tmpll); return 7;
    }
    std::printf("LLVM-IR backend: generated %s from %s\n", outfile, infile);
    std::printf("Temp IR: %s\n", tmpll);
    return 0;
}
