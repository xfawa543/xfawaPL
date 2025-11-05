#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define GET_PID() _getpid()
#else
#include <unistd.h>
#define GET_PID() getpid()
#endif

#define MAX_CODE_SIZE 65536
#define VERSION "1.0.0-a.0"

typedef enum {
    COMPILE_SUCCESS = 0,
    ERROR_USAGE,
    ERROR_FILE_OPEN,
    ERROR_FILE_SIZE,
    ERROR_MEMORY,
    ERROR_SYNTAX,
    ERROR_TEMP_FILE,
    ERROR_COMPILE_FAILED
} CompileResult;

void report_error(CompileResult err, const char* msg) {
    const char* errors[] = {
        "成功", "参数错误", "文件打开失败", "文件大小无效",
        "内存分配失败", "语法错误", "临时文件错误", "编译失败"
    };
    fprintf(stderr, "错误: %s - %s\n", errors[err], msg);
}

int parse_xf_program(const char* code, FILE* output) {
    const char* current = code;
    int found_main = 0;
    int brace_level = 0;

    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <locale.h>\n");
    fprintf(output, "#ifdef _WIN32\n");
    fprintf(output, "#include <windows.h>\n");
    fprintf(output, "#include <stdlib.h>\n");
    fprintf(output, "#endif\n\n");
    fprintf(output, "#ifdef _WIN32\n");
    fprintf(output, "void print_utf8(const char* s) {\n");
    fprintf(output, "    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);\n");
    fprintf(output, "    if (h == INVALID_HANDLE_VALUE) return;\n");
    fprintf(output, "    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);\n");
    fprintf(output, "    if (wlen > 0) {\n");
    fprintf(output, "        wchar_t* w = (wchar_t*)malloc(wlen * sizeof(wchar_t));\n");
    fprintf(output, "        MultiByteToWideChar(CP_UTF8, 0, s, -1, w, wlen);\n");
    fprintf(output, "        DWORD written = 0;\n");
    fprintf(output, "        WriteConsoleW(h, w, wlen - 1, &written, NULL);\n");
    fprintf(output, "        WriteConsoleW(h, L\"\\n\", 1, &written, NULL);\n");
    fprintf(output, "        free(w);\n");
    fprintf(output, "    }\n");
    fprintf(output, "}\n");
    fprintf(output, "#else\n");
    fprintf(output, "void print_utf8(const char* s) { printf(\\\"%%s\\\\n\\\", s); }\n");
    fprintf(output, "#endif\n\n");

    fprintf(output, "int main() {\n");
    fprintf(output, "    /* 设置 Windows 控制台为 UTF-8 (如果在 Windows 上运行) */\n");
    fprintf(output, "#ifdef _WIN32\n");
    fprintf(output, "    SetConsoleOutputCP(CP_UTF8);\n");
    fprintf(output, "    SetConsoleCP(CP_UTF8);\n");
    fprintf(output, "#endif\n");
    fprintf(output, "    setlocale(LC_ALL, \"\");\n");

    current = strchr(code, '{');
    if (!current) {
        report_error(ERROR_SYNTAX, "未找到类定义的大括号");
        return 0;
    }
    current++;

    while (*current) {
        if (strncmp(current, "fn main()", 9) == 0) {
            found_main = 1;
            current += 9;
            break;
        }
        current++;
    }

    if (!found_main) {
        report_error(ERROR_SYNTAX, "未找到fn main()函数");
        return 0;
    }

    current = strchr(current, '{');
    if (!current) {
        report_error(ERROR_SYNTAX, "未找到main函数体");
        return 0;
    }
    current++;

    brace_level = 1;

    int print_count = 0;
    while (*current && brace_level > 0) {
        while (isspace(*current)) current++;

        if (*current == '}') {
            brace_level--;
            if (brace_level == 0) break;
            current++;
            continue;
        }

        if (*current == '{') {
            brace_level++;
            current++;
            continue;
        }

        if (strncmp(current, "print(\"", 7) == 0) {
            const char* string_start = current + 7;
            const char* string_end = string_start;

            while (*string_end && *string_end != '"') {
                if (*string_end == '\\' && *(string_end + 1)) {
                    string_end += 2;
                } else {
                    string_end++;
                }
            }

            if (*string_end == '"') {
                const char* closing_paren = strchr(string_end + 1, ')');
                if (closing_paren) {
                    fprintf(output, "    print_utf8(\"");
                    for (const char* p = string_start; p < string_end; p++) {
                        if (*p == '"' || *p == '\\') fputc('\\', output);
                        fputc(*p, output);
                    }
                    fprintf(output, "\");\n");
                    print_count++;

                    current = closing_paren + 1;
                    continue;
                }
            }
        }

        current++;
    }

    if (brace_level != 0) {
        report_error(ERROR_SYNTAX, "大括号不匹配");
        return 0;
    }

    fprintf(output, "    return 0;\n}\n");
    return print_count > 0;
}

char* read_file_safely(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        report_error(ERROR_FILE_OPEN, filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0 || size > MAX_CODE_SIZE) {
        fclose(fp);
        report_error(ERROR_FILE_SIZE, "文件大小无效");
        return NULL;
    }

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        report_error(ERROR_MEMORY, "内存不足");
        return NULL;
    }

    fread(buffer, 1, size, fp);
    buffer[size] = '\0';
    fclose(fp);

    return buffer;
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("xfawaPL编译器 v%s\n", VERSION);
            return COMPILE_SUCCESS;
        }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("用法: %s 输入.xf -o 输出程序\n", argv[0]);
            return COMPILE_SUCCESS;
        }
    }

    if (argc != 4 || strcmp(argv[2], "-o") != 0) {
        report_error(ERROR_USAGE, "参数格式错误");
        return ERROR_USAGE;
    }

    char* code = read_file_safely(argv[1]);
    if (!code) return ERROR_FILE_OPEN;

    char tmpname[64];
    snprintf(tmpname, sizeof(tmpname), "temp_%d.c", GET_PID());
    FILE* c_file = fopen(tmpname, "wb");
    if (!c_file) {
        free(code);
        report_error(ERROR_TEMP_FILE, "无法创建临时文件");
        return ERROR_TEMP_FILE;
    }

    int parse_ok = parse_xf_program(code, c_file);
    fclose(c_file);
    free(code);

    if (!parse_ok) {
        remove(tmpname);
        report_error(ERROR_SYNTAX, "解析失败");
        return ERROR_SYNTAX;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "gcc -finput-charset=UTF-8 -fexec-charset=UTF-8 %s -o \"%s\"", tmpname, argv[3]);

    remove(argv[3]);

    int result = system(cmd);
    remove(tmpname);

    if (result == 0) {
        printf("编译成功: %s\n", argv[3]);
        return COMPILE_SUCCESS;
    } else {
        report_error(ERROR_COMPILE_FAILED, "编译失败");
        return ERROR_COMPILE_FAILED;
    }

}
