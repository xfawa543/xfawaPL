/*
 * Copyright (c) 2025 xfawaPL contributors
 * Licensed under the GNU General Public License v3.0 - see LICENSE for details.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <cctype>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/types.h>
#endif

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ToolOutputFile.h"

using namespace llvm;

// 全局标志用于控制行为
static int g_debug = 0;
static int g_keep_temp = 0;
static const char* VERSION = "1.0.0-a.3";

#define DEBUG_LOG(...) do { if (g_debug) std::fprintf(stderr, "[debug] " __VA_ARGS__); } while(0)

static void InitializeLLVMTargets() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
}

static Function* CreatePrintUTF8Function(Module* M, IRBuilder<>* Builder) {
    LLVMContext& Context = M->getContext();
    FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), 
                                         {Type::getInt8PtrTy(Context)}, false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, "print_utf8", M);
    BasicBlock* BB = BasicBlock::Create(Context, "entry", F);
    Builder->SetInsertPoint(BB);
    FunctionType* PutsType = FunctionType::get(Type::getInt32Ty(Context),
                                               {Type::getInt8PtrTy(Context)}, false);
    Function* PutsFunc = Function::Create(PutsType, Function::ExternalLinkage, "puts", M);
    Value* Arg = F->arg_begin();
    Builder->CreateCall(PutsFunc, Arg);
    Builder->CreateRetVoid();
    return F;
}
struct ModMap {
    std::string from;
    std::string to;
};

static std::string read_file_safely(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    f.seekg(0, std::ios::end);
    size_t sz = f.tellg();
    if (sz == 0 || sz > 65536) return "";
    f.seekg(0, std::ios::beg);
    std::string buf(sz, '\0');
    if (!f.read(&buf[0], sz)) return "";
    return buf;
}

static int load_mods(const std::string& dir, std::vector<ModMap>& maps) {
    int count = 0;
#ifdef _WIN32
    std::string pattern = dir + "\\*.xfmod";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::string path = dir + "\\" + fd.cFileName;
        DEBUG_LOG("load_mods: reading %s\n", path.c_str());
        std::string buf = read_file_safely(path);
        if (buf.empty()) { DEBUG_LOG("load_mods: read_file_safely returned empty for %s\n", path.c_str()); continue; }
        DEBUG_LOG("load_mods: buf[0..80]=%.80s\n", buf.c_str());
        const char* p = buf.c_str();
        while (*p) {
            while (*p && *p != '"') p++;
            if (!*p) break;
            const char* a = p + 1;
            const char* q = strchr(a, '"');
            if (!q) break;
            size_t llen = (size_t)(q - a);
            if (llen >= 127) llen = 127;
            std::string lhs(a, llen);
            p = q + 1;
            DEBUG_LOG("load_mods: found lhs='%s'\n", lhs.c_str());
            while (*p && *p != '=') p++;
            if (!*p) break;
            p++;
            while (*p && *p != '"') p++;
            if (!*p) break;
            const char* b = p + 1;
            q = strchr(b, '"');
            if (!q) break;
            size_t rlen = (size_t)(q - b);
            if (rlen >= 127) rlen = 127;
            std::string rhs(b, rlen);
            p = q + 1;
            DEBUG_LOG("load_mods: found rhs='%s'\n", rhs.c_str());
            if (count < 128) {
                ModMap m;
                m.from = lhs;
                m.to = rhs;
                maps.push_back(m);
                count++;
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir.c_str());
    if (!d) return 0;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
        std::string name = e->d_name;
        if (name.size() <= 6 || name.substr(name.size() - 6) != ".xfmod") continue;
        std::string path = dir + "/" + name;
        DEBUG_LOG("load_mods: reading %s\n", path.c_str());
        std::string buf = read_file_safely(path);
        if (buf.empty()) { DEBUG_LOG("load_mods: read_file_safely returned empty for %s\n", path.c_str()); continue; }
        DEBUG_LOG("load_mods: buf[0..80]=%.80s\n", buf.c_str());
        const char* p = buf.c_str();
        while (*p) {
            while (*p && *p != '"') p++;
            if (!*p) break;
            const char* a = p + 1;
            const char* q = strchr(a, '"');
            if (!q) break;
            size_t llen = (size_t)(q - a);
            if (llen >= 127) llen = 127;
            std::string lhs(a, llen);
            p = q + 1;
            while (*p && *p != '=') p++;
            if (!*p) break;
            p++;
            while (*p && *p != '"') p++;
            if (!*p) break;
            const char* b = p + 1;
            q = strchr(b, '"');
            if (!q) break;
            size_t rlen = (size_t)(q - b);
            if (rlen >= 127) rlen = 127;
            std::string rhs(b, rlen);
            p = q + 1;
            if (count < 128) {
                ModMap m;
                m.from = lhs;
                m.to = rhs;
                maps.push_back(m);
                count++;
            }
        }
    }
    closedir(d);
#endif
    return count;
}

static std::string apply_mods(const std::string& src, const std::vector<ModMap>& maps) {
    if (maps.empty()) return src;
    std::string out;
    out.reserve(src.size() + 1024);
    const char* p = src.c_str();
    while (*p) {
        if (*p == '"') {
            const char* q = p + 1;
            while (*q && !(*q == '"' && *(q-1) != '\\')) q++;
            size_t len = (*q == '"') ? (q - p + 1) : (q - p);
            out.append(p, len);
            p += len;
            continue;
        }
        if (std::isalpha((unsigned char)*p) || *p == '_') {
            const char* s = p;
            while (std::isalnum((unsigned char)*p) || *p == '_') p++;
            size_t tl = p - s;
            std::string tok(s, tl);
            bool rep = false;
            for (const auto& m : maps) {
                if (tok == m.from) {
                    out += m.to;
                    rep = true;
                    break;
                }
            }
            if (!rep) out += tok;
            continue;
        }
        out += *p++;
    }
    return out;
}

#define MAX_RANGE_ELEMENTS 4096

static std::string trim(const std::string& s) {
    size_t a = 0; while (a < s.size() && isspace((unsigned char)s[a])) a++;
    size_t b = s.size(); while (b > a && isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b-a);
}

static std::string escape_bytes_as_c_string(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c >= 0x20 && c <= 0x7E) out += (char)c;
        else {
            char hex[5];
            std::snprintf(hex, sizeof(hex), "\\x%02X", c);
            out += hex;
        }
    }
    return out;
}

static int parse_range(const std::string& s, int* out_start, int* out_end, int* out_step) {
    size_t p = 0;
    while (p < s.size() && isspace((unsigned char)s[p])) p++;
    size_t dots = s.find("...", p);
    if (dots == std::string::npos) return 0;
    std::string left = s.substr(p, dots - p);
    p = dots + 3;
    size_t colon = s.find(':', p);
    std::string right = (colon == std::string::npos) ? s.substr(p) : s.substr(p, colon - p);
    int step = 1;
    if (colon != std::string::npos) {
        step = std::atoi(s.substr(colon + 1).c_str());
        if (step <= 0) return 0;
    }
    int a = std::atoi(left.c_str());
    int b = std::atoi(right.c_str());
    if (b < a) return 0;
    long cnt = ((long)(b - a) / step) + 1;
    if (cnt <= 0 || cnt > MAX_RANGE_ELEMENTS) return -1;
    *out_start = a; *out_end = b; *out_step = step;
    return 1;
}

static std::string make_fn_name(const std::string& block, const std::string& fn) {
    std::string out = block + "_" + fn;
    for (size_t i = 0; i < out.size(); ++i) {
        if (!std::isalnum((unsigned char)out[i]) && out[i] != '_') out[i] = '_';
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <input.xf> [-o output_exe] [--mods-dir <dir>] [--debug] [--keep-temp] [--emit-ir <file>]\n", argv[0]);
        std::fprintf(stderr, "Version: %s\n", VERSION);
        return 1;
    }
    
    const char* infile = nullptr;
    const char* outfile = nullptr;
    const char* modsdir = "mods";
    const char* emit_ir_file = nullptr;
    int self_test = 0;
    
    // 重置全局标志
    g_debug = 0;
    g_keep_temp = 0;
    
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            outfile = argv[++i];
        }
        else if (std::strcmp(argv[i], "--mods-dir") == 0 && i+1 < argc) {
            modsdir = argv[++i];
        }
        else if (std::strcmp(argv[i], "--debug") == 0) {
            g_debug = 1;
            DEBUG_LOG("Debug mode enabled\n");
        }
        else if (std::strcmp(argv[i], "--keep-temp") == 0) {
            g_keep_temp = 1;
            DEBUG_LOG("Keeping temporary files\n");
        }
        else if (std::strcmp(argv[i], "--emit-ir") == 0 && i+1 < argc) {
            emit_ir_file = argv[++i];
            DEBUG_LOG("Will emit IR to: %s\n", emit_ir_file);
        }
        else if (std::strcmp(argv[i], "--self-test") == 0) {
            self_test = 1;
        }
        else if (argv[i][0] == '-') {
            // 忽略未知选项，避免崩溃
            DEBUG_LOG("Unknown option: %s\n", argv[i]);
        }
        else if (!infile) {
            infile = argv[i];
        }
    }
    
    // 自测试模式
    if (self_test) {
        DEBUG_LOG("Running self-test...\n");
        std::fprintf(stderr, "Self-test not implemented yet\n");
        return 0;
    }
    
    if (!infile) {
        std::fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }
    
    DEBUG_LOG("Input: '%s', Output: '%s', Mods dir: '%s'\n", 
              infile, outfile ? outfile : "(default)", modsdir ? modsdir : "(null)");
    std::ifstream ifs(infile, std::ios::binary);
        if (!ifs) { std::fprintf(stderr, "Error: Cannot open input file %s\n", infile); return 2; }
    std::ostringstream ss; ss << ifs.rdbuf(); std::string code = ss.str();
        DEBUG_LOG("read input file, size=%zu\n", code.size());
        
        std::vector<ModMap> maps;
        if (modsdir) {
            int mcount = load_mods(modsdir, maps);
            DEBUG_LOG("load_mods returned %d\n", mcount);
            if (mcount > 0) {
                code = apply_mods(code, maps);
                DEBUG_LOG("mods applied, code len=%zu\n", code.size());
            }
        }
    std::string functions;
    std::string entry_fn;
    bool need_time = false;
    
    const char* scan = code.c_str();
    while (1) {
        const char* ob = strchr(scan, '#');
        if (!ob) break;
        const char* name_start = ob + 1;
        while (*name_start && isspace((unsigned char)*name_start)) name_start++;
        const char* name_end = name_start;
        while (*name_end && (isalnum((unsigned char)*name_end) || *name_end == '_')) name_end++;
        if (name_start == name_end) { scan = ob + 1; continue; }
        std::string blockname(name_start, name_end - name_start);
        const char* oblock = strchr(name_end, '{');
        if (!oblock) { scan = name_end; continue; }
        const char* p = oblock + 1;
        int lvl = 1;
        const char* start = p;
        while (*p && lvl > 0) {
            if (*p == '{') lvl++;
            else if (*p == '}') lvl--;
            p++;
    }
        if (lvl != 0) break;
        const char* end = p - 1;
        
        const char* line = start;
        while (line < end) {
            const char* le = line;
            while (le < end && *le != '\n') le++;
            const char* s = line;
            while (s < le && isspace((unsigned char)*s)) s++;
            const char* e = le;
            while (e > s && isspace((unsigned char)e[-1])) e--;
            
            // remove // comments
            const char* comment = strstr(s, "//");
            if (comment && comment < e) {
                e = comment;
            }
            
            size_t llen = e - s;
            if (llen > 0) {
                if (llen >= 3 && strncmp(s, "fn", 2) == 0 && isspace((unsigned char)s[2])) {
                    DEBUG_LOG("found fn line: '%.80s'\n", s);
                    const char* fnstart = s + 2;
                    while (fnstart < e && isspace((unsigned char)*fnstart)) fnstart++;
                    const char* fnend = fnstart;
                    while (fnend < e && (isalnum((unsigned char)*fnend) || *fnend == '_')) fnend++;
                    std::string fnname(fnstart, fnend - fnstart);
                    const char* brace = strchr(s, '{');
                    if (!brace) { line = le + 1; continue; }
                    const char* q = brace + 1;
                    int l = 1;
                    const char* bodystart = q;
                    while (q < end && l > 0) {
                        if (*q == '{') l++;
                        else if (*q == '}') l--;
                        q++;
                    }
                    const char* bodyend = q - 1;
                    std::string fname = make_fn_name(blockname, fnname);
                    if (entry_fn.empty()) entry_fn = fname;
                    if (fnname == "call" || fnname == "main" || fnname == "Test" || fnname == "you_function_name") {
                        entry_fn = fname;
                    }
                    functions += "void " + fname + "(void) {\n";
                    const char* L = bodystart;
                    while (L < bodyend) {
                        const char* NL = L;
                        while (NL < bodyend && *NL != '\n') NL++;
                        const char* ss = L;
                        while (ss < NL && isspace((unsigned char)*ss)) ss++;
                        const char* ee = NL;
                        while (ee > ss && isspace((unsigned char)ee[-1])) ee--;
                        
                        // remove // comments
                        const char* comment = strstr(ss, "//");
                        if (comment && comment < ee) {
                            ee = comment;
                        }
                        
                        size_t lon = ee - ss;
                        if (lon > 0) {
                            if (ss[0] == '$') {
                                const char* at = (const char*)memchr(ss, '@', lon);
                                if (at) {
                                    size_t bnl = at - (ss + 1);
                                    std::string bname(ss + 1, bnl);
                                    size_t fnl2 = ee - at - 1;
                                    std::string f2(at + 1, fnl2);
                                    std::string calln = make_fn_name(bname, f2);
                                    functions += "    " + calln + "();\n";
                                }
                            } else if (lon >= 6 && strncmp(ss, "print(", 6) == 0) {
                                const char* qs = strchr(ss, '"');
                                if (qs) {
                                    const char* qe = qs + 1;
                                    while (*qe && *qe != '"') {
                                        if (*qe == '\\' && *(qe+1)) qe += 2;
                                        else qe++;
                                    }
                                    if (*qe == '"') {
                                        size_t ql = qe - qs - 1;
                                        std::string lit(qs + 1, ql);
                                        std::string esc = escape_bytes_as_c_string(lit);
                                        functions += "    print_utf8(\"" + esc + "\");\n";
                                    }
                                }
                            } else if (lon > 2 && memchr(ss, '=', lon)) {
                                const char* eq = (const char*)memchr(ss, '=', lon);
                                const char* idend = eq;
                                while (idend > ss && isspace((unsigned char)idend[-1])) idend--;
                                const char* idstart = idend;
                                while (idstart > ss && (isalnum((unsigned char)idstart[-1]) || idstart[-1] == '_')) idstart--;
                                size_t idl = idend - idstart;
                                std::string ident(idstart, idl);
                                const char* valstart = eq + 1;
                                while (valstart < ee && isspace((unsigned char)*valstart)) valstart++;
                                const char* valend = ee;
                                while (valend > valstart && isspace((unsigned char)valend[-1])) valend--;
                                std::string val(valstart, valend - valstart);
                                if (val.find("random[") == 0 || val.find("rnd[") == 0) {
                                    size_t br = val.find('[');
                                    if (br != std::string::npos) {
                                        size_t rb = val.find(']', br);
                                        if (rb != std::string::npos) {
                                            std::string inner = val.substr(br + 1, rb - br - 1);
                                            int a, b, step;
                                            int pr = parse_range(inner, &a, &b, &step);
                                            if (pr == -1) {
                                                std::fprintf(stderr, "Error: Range too large for %s.%s (max %d elements)\n", blockname.c_str(), ident.c_str(), MAX_RANGE_ELEMENTS);
                                                L = NL + 1;
                                                continue;
                                            }
                                            need_time = true;
                                            char buf[512];
                                            std::snprintf(buf, sizeof(buf), "    int %s = (rand() %% (%d - %d + 1)) + %d;\n", ident.c_str(), b, a, a);
                                            functions += buf;
                                        }
                                    }
                                } else if (val.find("sequential[") == 0 || val.find("seq[") == 0 || val.find("reciprocal[") == 0 || val.find("rcp[") == 0) {
                                    size_t br = val.find('[');
                                    if (br != std::string::npos) {
                                        size_t rb = val.find(']', br);
                                        if (rb != std::string::npos) {
                                            std::string inner = val.substr(br + 1, rb - br - 1);
                                            int a, b, step;
                                            int pr = parse_range(inner, &a, &b, &step);
                                            if (pr == -1) {
                                                std::fprintf(stderr, "Error: Range too large for %s.%s (max %d elements)\n", blockname.c_str(), ident.c_str(), MAX_RANGE_ELEMENTS);
                                                L = NL + 1;
                                                continue;
                                            }
                                            if (pr == 0) {
                                                std::fprintf(stderr, "Error: Invalid range syntax for %s.%s\n", blockname.c_str(), ident.c_str());
                                                L = NL + 1;
                                                continue;
                                            }
                                            char idxname[128];
                                            std::snprintf(idxname, sizeof(idxname), "__seq_%s_idx", ident.c_str());
                                            char buf[512];
                                            if (val.find("reciprocal") == 0 || val.find("rcp") == 0) {
                                                std::snprintf(buf, sizeof(buf), "    static int %s = 0; int %s = %d - (%s++ * %d); if (%s < %d) %s = 0;\n", idxname, ident.c_str(), b, idxname, step, ident.c_str(), a, idxname);
                                            } else {
                                                std::snprintf(buf, sizeof(buf), "    static int %s = 0; int %s = %d + (%s++ * %d); if (%s > %d) %s = 0;\n", idxname, ident.c_str(), a, idxname, step, ident.c_str(), b, idxname);
                                            }
                                            functions += buf;
                                        }
                                    }
                                } else {
                                    char buf[512];
                                    std::snprintf(buf, sizeof(buf), "    int %s = %s;\n", ident.c_str(), val.c_str());
                                    functions += buf;
                                }
                            } else if (lon >= 2 && strncmp(ss, "if", 2) == 0 && isspace((unsigned char)ss[2])) {
                                const char* bpos = (const char*)memchr(ss, '{', lon);
                                if (bpos) {
                                    const char* conds = ss + 2;
                                    while (conds < (ss + lon) && isspace((unsigned char)*conds)) conds++;
                                    const char* cend = bpos;
                                    while (cend > conds && isspace((unsigned char)cend[-1])) cend--;
                                    std::string cond(conds, cend - conds);
                                    char buf[512];
                                    std::snprintf(buf, sizeof(buf), "    if (%s) {\n", cond.c_str());
                                    functions += buf;
                                    const char* ibody = bpos + 1;
                                    int ilvl = 1;
                                    const char* iq = ibody;
                                    while (iq < bodyend && ilvl > 0) {
                                        if (*iq == '{') ilvl++;
                                        else if (*iq == '}') ilvl--;
                                        iq++;
                                    }
                                    const char* iend = iq - 1;
                                    const char* iL = ibody;
                                    while (iL < iend) {
                                        const char* iNL = iL;
                                        while (iNL < iend && *iNL != '\n') iNL++;
                                        if (iNL > iL) {
                                            std::string iline(iL, iNL - iL);
                                            std::string itrimmed = trim(iline);
                                            if (!itrimmed.empty() && itrimmed != "}") {
                                                functions += "        " + iline + "\n";
                                            }
                                        }
                                        iL = iNL + 1;
                                    }
                                    functions += "    }\n";
                                    L = iend;
                                    continue;
                                }
                            } else if (lon >= 7 && strncmp(ss, "else if", 7) == 0) {
                                const char* conds = ss + 7;
                                while (conds < ee && isspace((unsigned char)*conds)) conds++;
                                const char* bpos = (const char*)memchr(conds, '{', ee - conds);
                                if (bpos) {
                                    const char* cend = bpos;
                                    while (cend > conds && isspace((unsigned char)cend[-1])) cend--;
                                    std::string cond(conds, cend - conds);
                                    char buf[512];
                                    std::snprintf(buf, sizeof(buf), "    else if (%s) {\n", cond.c_str());
                                    functions += buf;
                                    const char* ibody = bpos + 1;
                                    int ilvl = 1;
                                    const char* iq = ibody;
                                    while (iq < bodyend && ilvl > 0) {
                                        if (*iq == '{') ilvl++;
                                        else if (*iq == '}') ilvl--;
                                        iq++;
                                    }
                                    const char* iend = iq - 1;
                                    const char* iL = ibody;
                                    while (iL < iend) {
                                        const char* iNL = iL;
                                        while (iNL < iend && *iNL != '\n') iNL++;
                                        if (iNL > iL) {
                                            std::string iline(iL, iNL - iL);
                                            std::string itrimmed = trim(iline);
                                            if (!itrimmed.empty() && itrimmed != "}") {
                                                functions += "        " + iline + "\n";
                                            }
                                        }
                                        iL = iNL + 1;
                                    }
                                    functions += "    }\n";
                                    L = iend;
                                    continue;
                                }
                            } else if (lon >= 4 && strncmp(ss, "else", 4) == 0) {
                                const char* bpos = (const char*)memchr(ss, '{', lon);
                                if (bpos) {
                                    functions += "    else {\n";
                                    const char* ibody = bpos + 1;
                                    int ilvl = 1;
                                    const char* iq = ibody;
                                    while (iq < bodyend && ilvl > 0) {
                                        if (*iq == '{') ilvl++;
                                        else if (*iq == '}') ilvl--;
                                        iq++;
                                    }
                                    const char* iend = iq - 1;
                                    const char* iL = ibody;
                                    while (iL < iend) {
                                        const char* iNL = iL;
                                        while (iNL < iend && *iNL != '\n') iNL++;
                                        if (iNL > iL) {
                                            std::string iline(iL, iNL - iL);
                                            std::string itrimmed = trim(iline);
                                            if (!itrimmed.empty() && itrimmed != "}") {
                                                functions += "        " + iline + "\n";
    }
                                        }
                                        iL = iNL + 1;
                                    }
                                    functions += "    }\n";
                                    L = iend;
                                    continue;
                                }
                            } else if (ss[0] == '}') {
                                functions += "\n";
                            }
                        }
                        L = NL + 1;
                    }
                    functions += "}\n";
                    line = bodyend + 1;
                    continue;
                }
            }
            line = le + 1;
        }
        scan = end + 1;
    }
    
    if (functions.empty() && entry_fn.empty()) {
        std::fprintf(stderr, "Error: No functions found in code\n");
        return 5;
    }
    DEBUG_LOG("parsed functions, entry_fn='%s'\n", entry_fn.c_str());
    
    InitializeLLVMTargets();
    
    LLVMContext Context;
    std::unique_ptr<Module> M = std::make_unique<Module>("xfawa_module", Context);
    IRBuilder<> Builder(Context);
    
    // Create print_utf8 function declaration
    Function* PrintUTF8Func = CreatePrintUTF8Function(M.get(), &Builder);
    
    // Create function type for our user functions (void return, no arguments)
    FunctionType* VoidFuncType = FunctionType::get(Type::getVoidTy(Context), false);
    
    // Create a map to keep track of created functions
    std::map<std::string, Function*> created_functions;
    
    // Parse and create functions using LLVM IR
    std::istringstream func_stream(functions);
    std::string line;
    std::string current_func_name;
    Function* current_func = nullptr;
    BasicBlock* current_bb = nullptr;
    
    while (std::getline(func_stream, line)) {
        // Check for function definition start
        if (line.find("void ") == 0 && line.find("() {") != std::string::npos) {
            size_t start = 5; // Skip "void "
            size_t end = line.find("() {");
            current_func_name = line.substr(start, end - start);
            
            // Create new function
            current_func = Function::Create(VoidFuncType, Function::InternalLinkage, current_func_name, M.get());
            created_functions[current_func_name] = current_func;
            current_bb = BasicBlock::Create(Context, "entry", current_func);
            Builder.SetInsertPoint(current_bb);
            
            DEBUG_LOG("Created function: %s\n", current_func_name.c_str());
        }
        // Check for function end
        else if (line == "}") {
            if (current_func) {
                // Add return instruction
                Builder.CreateRetVoid();
                current_func = nullptr;
                current_bb = nullptr;
            }
        }
        // Handle print statements
        else if (line.find("print_utf8(") != std::string::npos) {
            if (current_func && line.find('"') != std::string::npos) {
                size_t start = line.find('"') + 1;
                size_t end = line.rfind('"');
                if (start < end) {
                    std::string str = line.substr(start, end - start);
                    // Create global string constant
                    Constant* StrConstant = ConstantDataArray::getString(Context, str);
                    GlobalVariable* GV = new GlobalVariable(*M, StrConstant->getType(), true, 
                                                          GlobalValue::PrivateLinkage, StrConstant);
                    // Create GEP to get pointer to the first character
                    Value* Zero = Constant::getNullValue(Type::getInt64Ty(Context));
                    Value* indices[] = { Zero, Zero };
                    Value* StrPtr = Builder.CreateGEP(GV->getType()->getPointerElementType(), GV, indices);
                    // Call print_utf8 function
                    Builder.CreateCall(PrintUTF8Func, {StrPtr});
                }
            }
        }
        // Handle variable declarations
        else if (line.find("int ") != std::string::npos && line.find(" = ") != std::string::npos) {
            if (current_func) {
                size_t int_start = line.find("int ") + 4;
                size_t eq_pos = line.find(" = ");
                std::string var_name = line.substr(int_start, eq_pos - int_start);
                var_name = trim(var_name);
                
                size_t val_start = eq_pos + 3;
                size_t val_end = line.find(';');
                std::string val_str = line.substr(val_start, val_end - val_start);
                val_str = trim(val_str);
                
                // Try to parse as integer constant
                try {
                    int val = std::stoi(val_str);
                    Value* IntVal = ConstantInt::get(Type::getInt32Ty(Context), val);
                    Value* Alloca = Builder.CreateAlloca(Type::getInt32Ty(Context), nullptr, var_name);
                    Builder.CreateStore(IntVal, Alloca);
                } catch (...) {
                    // For non-constant values, we'll just create an alloca for now
                    // TODO: Implement expression evaluation
                    Builder.CreateAlloca(Type::getInt32Ty(Context), nullptr, var_name);
                }
            }
        }
        // Handle if statements
        else if (line.find("if (") == 0 && line.find(") {") != std::string::npos) {
            if (current_func) {
                // Extract condition
                size_t cond_start = line.find('(') + 1;
                size_t cond_end = line.rfind(')');
                std::string condition = line.substr(cond_start, cond_end - cond_start);
                
                // Create basic blocks for if statement
                BasicBlock* IfBlock = BasicBlock::Create(Context, "if.then", current_func);
                BasicBlock* ElseBlock = BasicBlock::Create(Context, "if.else", current_func);
                BasicBlock* MergeBlock = BasicBlock::Create(Context, "if.merge", current_func);
                
                // Create condition (for simplicity, we'll just use a placeholder true condition)
                // TODO: Implement proper condition evaluation
                Value* CondVal = ConstantInt::get(Type::getInt1Ty(Context), 1);
                
                // Create branch instruction
                Builder.CreateCondBr(CondVal, IfBlock, ElseBlock);
                
                // Set insert point to if block
                Builder.SetInsertPoint(IfBlock);
                
                // TODO: Parse and generate code for the if body
                // For now, we'll just add a placeholder and unconditional branch
                Builder.CreateBr(MergeBlock);
                
                // Set insert point to else block
                Builder.SetInsertPoint(ElseBlock);
                Builder.CreateBr(MergeBlock);
                
                // Set insert point to merge block
                Builder.SetInsertPoint(MergeBlock);
            }
        }
        // Handle else if statements
        else if (line.find("else if (") == 0 && line.find(") {") != std::string::npos) {
            if (current_func) {
                // Extract condition
                size_t cond_start = line.find('(') + 1;
                size_t cond_end = line.rfind(')');
                std::string condition = line.substr(cond_start, cond_end - cond_start);
                
                // Create new else-if block
                BasicBlock* ElseIfBlock = BasicBlock::Create(Context, "elseif.then", current_func);
                BasicBlock* ElseBlock = BasicBlock::Create(Context, "elseif.else", current_func);
                BasicBlock* MergeBlock = BasicBlock::Create(Context, "elseif.merge", current_func);
                
                // Create condition (placeholder true)
                Value* CondVal = ConstantInt::get(Type::getInt1Ty(Context), 1);
                
                // Create branch instruction
                Builder.CreateCondBr(CondVal, ElseIfBlock, ElseBlock);
                
                // Set insert point to else-if block
                Builder.SetInsertPoint(ElseIfBlock);
                Builder.CreateBr(MergeBlock);
                
                // Set insert point to else block
                Builder.SetInsertPoint(ElseBlock);
                Builder.CreateBr(MergeBlock);
                
                // Set insert point to merge block
                Builder.SetInsertPoint(MergeBlock);
            }
        }
        // Handle else statements
        else if (line.find("else {") == 0) {
            if (current_func) {
                // Create else block
                BasicBlock* ElseBlock = BasicBlock::Create(Context, "pure.else", current_func);
                BasicBlock* MergeBlock = BasicBlock::Create(Context, "pure.merge", current_func);
                
                // Set insert point to else block
                Builder.SetInsertPoint(ElseBlock);
                Builder.CreateBr(MergeBlock);
                
                // Set insert point to merge block
                Builder.SetInsertPoint(MergeBlock);
            }
            }
        }
    }
    
    // Create main function that calls the entry function
    FunctionType* MainType = FunctionType::get(Type::getInt32Ty(Context), false);
    Function* MainFunc = Function::Create(MainType, Function::ExternalLinkage, "main", M.get());
    BasicBlock* MainBB = BasicBlock::Create(Context, "entry", MainFunc);
    Builder.SetInsertPoint(MainBB);
    
    // Call the entry function if it exists
    if (!entry_fn.empty() && created_functions.find(entry_fn) != created_functions.end()) {
        Builder.CreateCall(created_functions[entry_fn]);
    }
    
    // Return 0 from main
    Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(Context), 0));
    
    // Verify the module
    if (verifyModule(*M, &errs())) {
        std::fprintf(stderr, "Error: LLVM module verification failed\n");
        return 6;
    }
    
    DEBUG_LOG("LLVM IR generated successfully\n");
    
    if (!outfile) {
        std::string base(infile);
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        base += "_llvm.exe";
        outfile = strdup(base.c_str());
    }
    
    std::string TargetTriple = sys::getDefaultTargetTriple();
    M->setTargetTriple(TargetTriple);
    
    std::string Error;
    const Target* TheTarget = TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!TheTarget) {
        std::fprintf(stderr, "Error: %s\n", Error.c_str());
        return 7;
    }
    
    TargetOptions opt;
    std::unique_ptr<TargetMachine> TM(TheTarget->createTargetMachine(
        TargetTriple, "generic", "", opt, Reloc::PIC_));
    
    M->setDataLayout(TM->createDataLayout());
    
    std::string objfile = std::string(outfile) + ".o";
    std::error_code EC;
    raw_fd_ostream dest(objfile, EC, sys::fs::OF_None);
    if (EC) {
        std::fprintf(stderr, "Error: Could not open file: %s\n", EC.message().c_str());
        return 8;
    }
    
    legacy::PassManager PM;
    if (TM->addPassesToEmitFile(PM, dest, nullptr, CodeGenFileType::CGFT_ObjectFile)) {
        std::fprintf(stderr, "Error: TargetMachine can't emit a file of this type\n");
        return 9;
    }
    
    PM.run(*M);
    dest.flush();
    dest.close();
    
    DEBUG_LOG("LLVM object file generated: %s\n", objfile.c_str());
    
    std::string linker = "lld";
    std::string cmd;
#ifdef _WIN32
    cmd = linker + " /OUT:\"" + std::string(outfile) + "\" \"" + objfile + "\"";
#else
    cmd = linker + " -o \"" + std::string(outfile) + "\" \"" + objfile + "\"";
#endif
    int rc = std::system(cmd.c_str());
    
    if (rc != 0) {
#ifdef _WIN32
        linker = "link";
        cmd = linker + " /OUT:\"" + std::string(outfile) + "\" \"" + objfile + "\"";
#else
        linker = "ld";
        cmd = linker + " -o \"" + std::string(outfile) + "\" \"" + objfile + "\"";
#endif
        rc = std::system(cmd.c_str());
    }
    
    if (rc != 0) {
        std::fprintf(stderr, "Error: Linking failed. Object file: %s\n", objfile.c_str());
        if (!g_keep_temp) {
            std::remove(objfile.c_str());
        }
        return 10;
    }
    
    if (!g_keep_temp) {
        std::remove(objfile.c_str());
        DEBUG_LOG("removed temp object file: %s\n", objfile.c_str());
    } else {
        std::printf("Temp object file kept: %s\n", objfile.c_str());
    }
    
    std::printf("Generated: %s\n", outfile);
    DEBUG_LOG("LLVM machine code generated and linked successfully\n");
    
    return 0;
}
