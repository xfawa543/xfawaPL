#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <io.h>
#define GET_PID() _getpid()
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define GET_PID() getpid()
#endif

#define MAX_CODE_SIZE 65536
#define VERSION "1.0.0-a.1"

typedef enum { EXIT_OK = 0, EXIT_USAGE=1, EXIT_FILE=2, EXIT_SIZE=3, EXIT_MEM=4, EXIT_SYNTAX=5, EXIT_TEMP=6, EXIT_COMPILE=7 } ExitCode;

static void report_error(ExitCode code, const char* msg) {
    const char* names[] = { "成功", "参数错误", "文件或打开失败", "文件大小无效", "内存不足", "语法错误", "临时文件错误", "编译失败" };
    fprintf(stderr, "错误: %s - %s\n", names[(int)code], msg ? msg : "");
}

#define MAX_ERRORS 128
static int errors_count = 0;
static char errors_msg[MAX_ERRORS][256];
static const char* errors_pos[MAX_ERRORS];

static void record_error(const char* base, const char* pos, const char* fmt, ...) {
    if (errors_count >= MAX_ERRORS) return;
    va_list ap; va_start(ap, fmt);
    vsnprintf(errors_msg[errors_count], sizeof(errors_msg[0]), fmt, ap);
    va_end(ap);
    errors_pos[errors_count] = pos ? pos : base;
    errors_count++;
}

/* forward declaration for self-test runner (implemented later) */
static int run_self_tests(void);

static const char* mods_dir = NULL;

static char* read_file_safely(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { report_error(EXIT_FILE, path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > MAX_CODE_SIZE) { fclose(f); report_error(EXIT_SIZE, "文件大小无效"); return NULL; }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); report_error(EXIT_MEM, "malloc 失败"); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { fclose(f); free(buf); report_error(EXIT_FILE, "读取失败"); return NULL; }
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static void emit_escaped_string(FILE* out, const char* s, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') { fputc('\\', out); fputc(c, out); }
        else if (c == '\n') fputs("\\n", out);
        else if (c == '\r') fputs("\\r", out);
        else if (c == '\t') fputs("\\t", out);
        else fputc(c, out);
    }
}

static void write_preamble(FILE* out) {
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <locale.h>\n");
    fprintf(out, "#ifdef _WIN32\n");
    fprintf(out, "#include <windows.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#endif\n\n");
    fprintf(out, "#ifdef _WIN32\n");
    fprintf(out, "void print_utf8(const char* s) {\n");
    fprintf(out, "    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);\n");
    fprintf(out, "    if (h == INVALID_HANDLE_VALUE) return;\n");
    fprintf(out, "    /* convert UTF-8 to wide */\n");
    fprintf(out, "    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);\n");
    fprintf(out, "    if (wlen <= 0) { printf(\"%%s\\n\", s); return; }\n");
    fprintf(out, "    wchar_t* w = (wchar_t*)malloc(wlen * sizeof(wchar_t));\n");
    fprintf(out, "    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, wlen);\n");
    fprintf(out, "    /* If stdout is a console, write wide directly. Otherwise convert to console codepage and printf. */\n");
    fprintf(out, "    DWORD mode;\n");
    fprintf(out, "    if (GetConsoleMode(h, &mode)) {\n");
    fprintf(out, "        DWORD written = 0;\n");
    fprintf(out, "        WriteConsoleW(h, w, wlen - 1, &written, NULL);\n");
    fprintf(out, "        WriteConsoleW(h, L\"\\n\", 1, &written, NULL);\n");
    fprintf(out, "        free(w);\n");
    fprintf(out, "        return;\n");
    fprintf(out, "    } else {\n");
    fprintf(out, "        UINT cp = GetConsoleOutputCP();\n");
    fprintf(out, "        int mblen = WideCharToMultiByte(cp, 0, w, -1, NULL, 0, NULL, NULL);\n");
    fprintf(out, "        if (mblen > 0) {\n");
    fprintf(out, "            char* mb = (char*)malloc((size_t)mblen);\n");
    fprintf(out, "            WideCharToMultiByte(cp, 0, w, -1, mb, mblen, NULL, NULL);\n");
    fprintf(out, "            fputs(mb, stdout); fputc('\\n', stdout);\n");
    fprintf(out, "            free(mb);\n");
    fprintf(out, "            free(w);\n");
    fprintf(out, "            return;\n");
    fprintf(out, "        }\n");
    fprintf(out, "    }\n");
    fprintf(out, "    free(w);\n");
    fprintf(out, "}\n");
    fprintf(out, "#else\n");
    fprintf(out, "void print_utf8(const char* s) { printf(\"%%s\\n\", s); }\n");
    fprintf(out, "#endif\n\n");

    fprintf(out, "int main(void) {\n");
    fprintf(out, "#ifdef _WIN32\n");
    fprintf(out, "    SetConsoleOutputCP(CP_UTF8);\n");
    fprintf(out, "    SetConsoleCP(CP_UTF8);\n");
    fprintf(out, "#endif\n");
    fprintf(out, "    setlocale(LC_ALL, \"\");\n\n");
}

static void print_error_context_with_lines(const char* base, const char* pos, int context_lines) {
    if (!base || !pos) return;
    int line_no = 1;
    for (const char* p = base; p < pos; ++p) if (*p == '\n') line_no++;
    const char* line_start = pos;
    while (line_start > base && line_start[-1] != '\n') line_start--;
    const char* start = line_start;
    for (int i = 0; i < context_lines; ++i) {
        const char* prev = start;
        if (prev == base) break;
        while (prev > base && prev[-1] != '\n') prev--;
        if (prev == start) break;
        start = prev;
    }
    const char* end = line_start;
    for (int i = 0; i <= context_lines; ++i) {
        const char* e = end;
        while (*e && *e != '\n') e++;
        if (*e == '\n') e++;
        end = e;
    }
    int start_line_no = 1; for (const char* p = base; p < start; ++p) if (*p == '\n') start_line_no++;
    fprintf(stderr, "错误上下文 (文件行 %d):\n", line_no);
    const char* cur = start;
    int cur_ln = start_line_no;
    while (cur < end && *cur) {
        fprintf(stderr, "%5d | ", cur_ln);
        const char* e = cur;
        while (e < end && *e && *e != '\n') e++;
        fwrite(cur, 1, (size_t)(e - cur), stderr);
        fprintf(stderr, "\n");
        if (cur <= pos && pos < e) {
            int col = (int)(pos - cur);
            fprintf(stderr, "      | ");
            for (int k = 0; k < col; ++k) fputc((cur[k] == '\t') ? '\t' : ' ', stderr);
            fprintf(stderr, "^\n");
        }
        if (*e == '\n') e++;
        cur = e; cur_ln++;
    }
}

static int parse_block_and_emit(const char* file_base, const char* src, FILE* out, char declared[][64], int* decl_count) {
    const char* cur = src;
    int brace_level = 1;
    while (*cur && brace_level > 0) {
        while (*cur && isspace((unsigned char)*cur)) cur++;
        if (!*cur) break;
        if (*cur == '{') { brace_level++; cur++; continue; }
        if (*cur == '}') { brace_level--; cur++; if (brace_level > 0) fprintf(out, "    }\n"); continue; }
        const char* line_start = cur;
        const char* line_end = strchr(line_start, '\n');
        if (!line_end) line_end = line_start + strlen(line_start);
        const char* le = line_end; while (le > line_start && isspace((unsigned char)le[-1])) le--;
        size_t llen = (size_t)(le - line_start);
        if (llen == 0) { cur = (line_end && *line_end) ? line_end + 1 : line_end; continue; }
        if (llen >= 7 && strncmp(line_start, "print(\"", 7) == 0) {
            const char* s = line_start + 7; const char* se = s;
            while (se < le && *se != '"') { if (*se == '\\' && (se+1) < le) se += 2; else se++; }
            if (se >= le) { record_error(file_base, s, "字符串未终止"); print_error_context_with_lines(file_base, s, 2); cur = (line_end && *line_end) ? line_end + 1 : line_end; continue; }
            fprintf(out, "    print_utf8(\"");
            emit_escaped_string(out, s, (size_t)(se - s));
            fprintf(out, "\");\n");
            cur = (line_end && *line_end) ? line_end + 1 : line_end;
            continue;
        }
        if (llen >= 2 && strncmp(line_start, "if", 2) == 0 && isspace((unsigned char)line_start[2])) {
            const char* brace = memchr(line_start, '{', llen);
            if (!brace) {
                const char* t = line_end + 1; while (t && *t && isspace((unsigned char)*t)) t++; if (t && *t == '{') brace = t; else { record_error(file_base, line_start, "if 语句缺少 '{'"); print_error_context_with_lines(file_base, line_start, 2); cur = (line_end && *line_end) ? line_end + 1 : line_end; continue; }
            }
            const char* cond_start = line_start + 2; while (cond_start < brace && isspace((unsigned char)*cond_start)) cond_start++;
            const char* cond_end = brace; while (cond_end > cond_start && isspace((unsigned char)cond_end[-1])) cond_end--;
            int clen = (int)(cond_end - cond_start);
            if (clen <= 0) { record_error(file_base, cond_start, "if 条件为空"); print_error_context_with_lines(file_base, cond_start, 2); cur = (line_end && *line_end) ? line_end + 1 : line_end; continue; }
            char cond[256]; if (clen >= (int)sizeof(cond)) clen = (int)sizeof(cond)-1; memcpy(cond, cond_start, clen); cond[clen] = '\0';
            fprintf(out, "    if (%s) {\n", cond);
            brace_level++;
            cur = brace + 1;
            continue;
        }
        if (llen >= 4 && strncmp(line_start, "else", 4) == 0) {
            const char* brace = memchr(line_start, '{', llen);
            if (!brace) {
                const char* t = line_end + 1; while (t && *t && isspace((unsigned char)*t)) t++; if (t && *t == '{') brace = t; else { record_error(file_base, line_start, "else 语句缺少 '{'"); print_error_context_with_lines(file_base, line_start, 2); cur = (line_end && *line_end) ? line_end + 1 : line_end; continue; }
            }
            fprintf(out, "    else {\n");
            brace_level++;
            cur = brace + 1;
            continue;
        }
        const char* eq = memchr(line_start, '=', llen);
        if (eq) {
            const char* id_end = eq; while (id_end > line_start && isspace((unsigned char)id_end[-1])) id_end--; const char* id_start = id_end; while (id_start > line_start && (isalnum((unsigned char)id_start[-1]) || id_start[-1] == '_')) id_start--; size_t idl = (size_t)(id_end - id_start);
            if (idl > 0 && idl < 64) {
                char ident[64] = {0}; if (idl >= sizeof(ident)) idl = sizeof(ident)-1; memcpy(ident, id_start, idl); ident[idl] = '\0';
                const char* val_start = eq + 1; while (val_start < line_start + (int)llen && isspace((unsigned char)*val_start)) val_start++; const char* val_end = line_start + llen; while (val_end > val_start && isspace((unsigned char)val_end[-1])) val_end--; size_t vlen = (size_t)(val_end - val_start); char value[256] = {0}; if (vlen >= sizeof(value)) vlen = sizeof(value)-1; memcpy(value, val_start, vlen); value[vlen] = '\0';
                int found = 0; for (int i = 0; i < *decl_count; ++i) { if (strcmp(declared[i], ident) == 0) { found = 1; break; } }
                if (!found) { if (*decl_count < 128) snprintf(declared[(*decl_count)++], 64, "%s", ident); fprintf(out, "    int %s = %s;\n", ident, value); } else { fprintf(out, "    %s = %s;\n", ident, value); }
                cur = (line_end && *line_end) ? line_end + 1 : line_end;
                continue;
            }
        }
        cur = (line_end && *line_end) ? line_end + 1 : line_end;
    }
    return 1;
}

static int parse_xf_program(const char* code, FILE* out) {
    write_preamble(out);
    const char* scan = code;
    int any_block = 0;
    char declared[128][64]; int decl_count = 0;
    while (1) {
        const char* ob = strchr(scan, '{');
        if (!ob) break;
        const char* q = ob + 1;
        int lvl = 1;
        while (*q && lvl > 0) { if (*q == '{') lvl++; else if (*q == '}') lvl--; q++; }
        if (lvl != 0) { record_error(code, ob, "大括号不匹配"); print_error_context_with_lines(code, ob, 2); break; }
        if (!parse_block_and_emit(code, ob + 1, out, declared, &decl_count)) return 0;
        any_block = 1;
        scan = q;
    }
    if (!any_block) { record_error(code, code, "未找到程序块"); return 0; }
    if (errors_count > 0) {
        return 0;
    }
    fprintf(out, "    return 0;\n}\n");
    return 1;
}

static int call_gcc(const char* tmpc, const char* outfile) {
    const char* gcc = "gcc";
    const char* argv[] = { "gcc", "-std=c11", "-O2", "-Wall", "-Wextra", "-finput-charset=UTF-8", "-fexec-charset=UTF-8", tmpc, "-o", outfile, NULL };
#ifdef _WIN32
    int rc = _spawnvp(_P_WAIT, gcc, (const char* const*)argv);
    return rc == 0 ? 0 : rc;
#else
    pid_t pid = fork(); if (pid < 0) return -1; if (pid == 0) { execvp(gcc, (char* const*)argv); _exit(127); }
    int status = 0; waitpid(pid, &status, 0); if (WIFEXITED(status)) return WEXITSTATUS(status); return -1;
#endif
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "用法: %s <input.xf> [-o output] [--emit-c <path>] [--keep-temp] [--mods-dir <dir>] [--self-test]\n", argv[0]); return 1; }
    const char* infile = NULL; const char* outfile = NULL; const char* emit_c_path = NULL; int keep_temp = 0; int run_tests = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--self-test") == 0) { run_tests = 1; continue; }
        if (strcmp(argv[i], "-o") == 0) { if (i+1 < argc) outfile = argv[++i]; }
        else if (strcmp(argv[i], "--emit-c") == 0) { if (i+1 < argc) emit_c_path = argv[++i]; }
        else if (strcmp(argv[i], "--keep-temp") == 0) { keep_temp = 1; }
        else if (strcmp(argv[i], "--mods-dir") == 0) { if (i+1 < argc) mods_dir = argv[++i]; }
        else if (argv[i][0] == '-') { }
        else if (!infile) infile = argv[i];
    }
    if (run_tests) return run_self_tests();
    if (!infile) { report_error(EXIT_USAGE, "未提供输入文件"); return 1; }
    char outbuf[512]; if (!outfile) { const char* dot = strrchr(infile, '.'); size_t len = dot ? (size_t)(dot - infile) : strlen(infile); if (len >= sizeof(outbuf)) len = sizeof(outbuf)-1; memcpy(outbuf, infile, len); outbuf[len] = '\0'; outfile = outbuf; }
    char* code = read_file_safely(infile); if (!code) return 1;
    char tmpc[512]; if (emit_c_path) snprintf(tmpc, sizeof(tmpc), "%s", emit_c_path); else snprintf(tmpc, sizeof(tmpc), "temp_%d.c", (int)GET_PID());
    FILE* out = fopen(tmpc, "wb"); if (!out) { report_error(EXIT_TEMP, "无法创建临时文件"); free(code); return 1; }
    int ok = parse_xf_program(code, out); fclose(out);
    if (!ok || errors_count > 0) {
        fprintf(stderr, "解析发现 %d 个错误:\n", errors_count);
        for (int i = 0; i < errors_count; ++i) {
            int line_no = 1; for (const char* p = code; p < errors_pos[i]; ++p) if (*p == '\n') line_no++;
            fprintf(stderr, "  %d: %s (line %d)\n", i+1, errors_msg[i], line_no);
            print_error_context_with_lines(code, errors_pos[i], 2);
        }
        if (!emit_c_path && !keep_temp) {
            fprintf(stderr, "保留临时文件 %s 以便调试\n", tmpc);
        } else {
            fprintf(stderr, "临时 C 文件已写入: %s\n", tmpc);
        }
        free(code);
        return 1;
    }
    free(code);
    int rc = call_gcc(tmpc, outfile);
    if (rc != 0) { report_error(EXIT_COMPILE, "gcc 返回非零状态"); fprintf(stderr, "编译命令: gcc %s -o %s (rc=%d)\n", tmpc, outfile, rc); if (!emit_c_path && !keep_temp) { fprintf(stderr, "保留临时文件 %s 以便调试\n", tmpc); } return 1; }
    if (!emit_c_path && !keep_temp) remove(tmpc);
    printf("已生成: %s (version %s)\n", outfile, VERSION);
    if (emit_c_path) fprintf(stderr, "已保存中间 C 文件到: %s\n", tmpc);
    return 0;
}

/* Run a shell command and capture stdout+stderr into a malloc'd buffer (caller frees). */
static char* run_command_capture(const char* cmd) {
    if (!cmd) return NULL;
    char buffer[4096];
    size_t cap = 8192;
    char* out = (char*)malloc(cap);
    if (!out) return NULL;
    out[0] = '\0';
    size_t len = 0;
#ifdef _WIN32
    /* use cmd /c to allow 2>&1 */
    char fullcmd[4096];
    snprintf(fullcmd, sizeof(fullcmd), "cmd /c \"%s 2>&1\"", cmd);
    FILE* f = _popen(fullcmd, "r");
#else
    char fullcmd[4096];
    snprintf(fullcmd, sizeof(fullcmd), "%s 2>&1", cmd);
    FILE* f = popen(fullcmd, "r");
#endif
    if (!f) { free(out); return NULL; }
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t r = strlen(buffer);
        if (len + r + 1 > cap) {
            cap = (cap + r + 1) * 2;
            char* n = (char*)realloc(out, cap);
            if (!n) { free(out); out = NULL; break; }
            out = n;
        }
        if (!out) break;
        memcpy(out + len, buffer, r); len += r; out[len] = '\0';
    }
#ifdef _WIN32
    _pclose(f);
#else
    pclose(f);
#endif
    return out;
}

/* Run built-in self tests similar to run_tests.ps1. Returns 0 on success */
static int run_self_tests(void) {
    struct test { const char* src; const char* out; const char** expects; int expect_count; };
    static const char* t1_expect[] = { "你好，世界！", "114514", "HelloWorld" };
    static const char* t2_expect[] = { "you 18" };
    static const char* t3_expect[] = { "你好，世界！", "114514", "HelloWorld", "18" };
    struct test tests[] = {
        { "hello.xf", "hello.exe", t1_expect, 3 },
        { "if_test.xf", "if_test.exe", t2_expect, 1 },
        { "duoblock.xf", "duoblock.exe", t3_expect, 4 }
    };
    int allOk = 1;
    char cwd[1024]; if (!getcwd(cwd, sizeof(cwd))) cwd[0] = '\0';
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        const char* src = tests[i].src;
        const char* outexe = tests[i].out;
        printf("\nTesting %s -> %s\n", src, outexe);
        /* read source */
        char* code = read_file_safely(src);
        if (!code) { fprintf(stderr, "Translator failed to read %s\n", src); allOk = 0; continue; }
        char tmpc[512]; snprintf(tmpc, sizeof(tmpc), "temp_self_%d_%zu.c", (int)GET_PID(), i);
        FILE* f = fopen(tmpc, "wb"); if (!f) { fprintf(stderr, "Cannot create %s\n", tmpc); free(code); allOk = 0; continue; }
        int ok = parse_xf_program(code, f); fclose(f);
        if (!ok) { fprintf(stderr, "Translator failed to parse %s\n", src); free(code); allOk = 0; continue; }
        free(code);
        int rc = call_gcc(tmpc, outexe);
        if (rc != 0) { fprintf(stderr, "gcc failed for %s (rc=%d)\n", outexe, rc); allOk = 0; continue; }
        /* run produced exe and capture output */
        char cmdline[1024];
#ifdef _WIN32
        snprintf(cmdline, sizeof(cmdline), "%s", outexe);
#else
        snprintf(cmdline, sizeof(cmdline), "./%s", outexe);
#endif
        char* outtxt = run_command_capture(cmdline);
        if (!outtxt) { fprintf(stderr, "Failed to run %s\n", outexe); allOk = 0; continue; }
        for (int j = 0; j < tests[i].expect_count; ++j) {
            if (!strstr(outtxt, tests[i].expects[j])) {
                fprintf(stderr, "Expected '%s' not found in output of %s\n", tests[i].expects[j], outexe);
                allOk = 0;
            }
        }
        free(outtxt);
        /* cleanup temp C file */
        remove(tmpc);
        if (!strstr(outexe, "keep")) {
            /* keep produced exe by default? match PS script which keeps them */
        }
    }
    if (allOk) printf("All tests passed.\n"); else fprintf(stderr, "Some tests failed.\n");
    return allOk ? 0 : 2;
}

/* forward declaration for self-test runner */
static int run_self_tests(void);
