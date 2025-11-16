#include <stdio.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
void print_utf8(const char* s) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return;
    /* convert UTF-8 to wide */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (wlen <= 0) { printf("%s\n", s); return; }
    wchar_t* w = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, wlen);
    /* If stdout is a console, write wide directly. Otherwise convert to console codepage and printf. */
    DWORD mode;
    if (GetConsoleMode(h, &mode)) {
        DWORD written = 0;
        WriteConsoleW(h, w, wlen - 1, &written, NULL);
        WriteConsoleW(h, L"\n", 1, &written, NULL);
        free(w);
        return;
    } else {
        UINT cp = GetConsoleOutputCP();
        int mblen = WideCharToMultiByte(cp, 0, w, -1, NULL, 0, NULL, NULL);
        if (mblen > 0) {
            char* mb = (char*)malloc((size_t)mblen);
            WideCharToMultiByte(cp, 0, w, -1, mb, mblen, NULL, NULL);
            fputs(mb, stdout); fputc('\n', stdout);
            free(mb);
            free(w);
            return;
        }
    }
    free(w);
}
#else
void print_utf8(const char* s) { printf("%s\n", s); }
#endif

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    setlocale(LC_ALL, "");

    int a = sequential[1...20]    # 顺序访问;
    int b = random[1...20]        # 随机访问;
    int c = reciprocal[1...20]    # 倒数访问;
    int d = seq[1...20]          # sequential → seq;
    int e = rnd[1...20]          # random → rnd;
    int f = rcp[1...20]          # reciprocal → rcp;
    int g = sequential[1...20];
    int h = random[1...20];
    int i = reciprocal[1...20];
    int j = seq[1...20];
    int k = rnd[1...20];
    int l = rcp[1...20];
    if (a == 1) {
    print_utf8("1");
    }
    if (b >= 20) {
    print_utf8("满分！！！");
    }
    else {
    print_utf8("继续加油");
    }
    return 0;
}
