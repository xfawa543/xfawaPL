#ifndef XF_LICENSE_HEADER
#define XF_LICENSE_HEADER
/*
 * Copyright (c) 2025 xfawaPL contributors
 * Licensed under the GNU General Public License v3.0 - see LICENSE for details.
 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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
#define MAX_RANGE_ELEMENTS 4096
#define VERSION "1.0.0-a.3"

/* Global flags for controlling behavior */
static int g_debug = 0;
static int g_keep_temp = 0;

/* Debug logging macro */
#define DEBUG_LOG(...) do { if (g_debug) fprintf(stderr, "[debug] " __VA_ARGS__); } while(0)

static char* read_file_safely(const char* path) {
    FILE* f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    if (sz <= 0 || sz > MAX_CODE_SIZE) { fclose(f); return NULL; }
    char* buf = malloc((size_t)sz + 1); if (!buf) { fclose(f); return NULL; }
    if (fread(buf,1,(size_t)sz,f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz]='\0'; fclose(f); return buf;
}

/* Check whether a byte buffer is valid UTF-8 */
static int is_valid_utf8(const unsigned char* s, size_t len) {
    size_t i = 0;
    while (i < len) {
        unsigned char c = s[i];
        if (c <= 0x7F) { i++; continue; }
        else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= len) return 0;
            if ((s[i+1] & 0xC0) != 0x80) return 0;
            if ((c & 0xFE) == 0xC0) return 0; /* overlong */
            i += 2; continue;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= len) return 0;
            if ((s[i+1] & 0xC0) != 0x80 || (s[i+2] & 0xC0) != 0x80) return 0;
            i += 3; continue;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= len) return 0;
            if ((s[i+1] & 0xC0) != 0x80 || (s[i+2] & 0xC0) != 0x80 || (s[i+3] & 0xC0) != 0x80) return 0;
            i += 4; continue;
        } else return 0;
    }
    return 1;
}

#ifdef _WIN32
/* Convert a buffer encoded in the current ANSI code page (CP_ACP) to UTF-8.
   Caller receives a newly malloc'ed buffer and must free it. Returns NULL on error. */
static char* convert_cp_acp_to_utf8(const char* inbuf, size_t inlen, size_t* outlen) {
    if (!inbuf) return NULL;
    int wlen = MultiByteToWideChar(CP_ACP, 0, inbuf, (int)inlen, NULL, 0);
    if (wlen <= 0) return NULL;
    wchar_t* wbuf = (wchar_t*)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) return NULL;
    if (MultiByteToWideChar(CP_ACP, 0, inbuf, (int)inlen, wbuf, wlen) == 0) { free(wbuf); return NULL; }
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, NULL, 0, NULL, NULL);
    if (ulen <= 0) { free(wbuf); return NULL; }
    char* out = (char*)malloc((size_t)ulen + 1);
    if (!out) { free(wbuf); return NULL; }
    if (WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, out, ulen, NULL, NULL) == 0) { free(out); free(wbuf); return NULL; }
    out[ulen] = '\0'; if (outlen) *outlen = (size_t)ulen; free(wbuf); return out;
}
#endif

/* Ensure buffer is valid UTF-8; if not and on Windows, convert from ANSI (CP_ACP) to UTF-8. Returns a malloc'ed buffer.
   Caller must free the returned pointer. On failure, returns NULL. */
static char* ensure_utf8_buffer(char* buf) {
    if (!buf) return NULL;
    size_t len = strlen(buf);
    if (is_valid_utf8((const unsigned char*)buf, len)) return buf; /* already UTF-8 */
#ifdef _WIN32
    size_t outlen = 0;
    char* conv = convert_cp_acp_to_utf8(buf, len, &outlen);
    if (conv) { free(buf); return conv; }
#endif
    /* unable to convert; just return original */
    return buf;
}

/* Escape a byte buffer into a C string literal body (no surrounding quotes).
   Keeps printable ASCII (0x20..0x7E) except '"' and '\\' as-is, escapes common
   controls, and emits \xHH for bytes >= 0x80 or other non-printable bytes.
   Returns malloc'ed string which caller must free. */
static char* escape_bytes_as_c_string(const char* in) {
    if (!in) return NULL;
    size_t len = strlen(in);
    size_t cap = len * 4 + 16;
    char* out = malloc(cap);
    if (!out) return NULL;
    size_t oi = 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"') { if (oi + 2 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = '\\'; out[oi++] = '"'; }
        else if (c == '\\') { if (oi + 2 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = '\\'; out[oi++] = '\\'; }
        else if (c == '\n') { if (oi + 2 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = '\\'; out[oi++] = 'n'; }
        else if (c == '\r') { if (oi + 2 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = '\\'; out[oi++] = 'r'; }
        else if (c == '\t') { if (oi + 2 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = '\\'; out[oi++] = 't'; }
        else if (c >= 0x20 && c <= 0x7E) { if (oi + 1 >= cap) { cap *= 2; out = realloc(out, cap); } out[oi++] = (char)c; }
        else { if (oi + 4 >= cap) { cap *= 2; out = realloc(out, cap); } unsigned int v = c; const char* hex = "0123456789ABCDEF"; out[oi++] = '\\'; out[oi++] = 'x'; out[oi++] = hex[(v>>4)&0xF]; out[oi++] = hex[v&0xF]; }
    }
    out[oi] = '\0'; return out;
}

/* Very small mod map loader: reads all mods/*.xfmod and extracts "a" = "b" pairs */
struct modmap { char from[128]; char to[128]; };

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>
#include <search.h>
#include <malloc.h>
#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <sys/stat.h>
#endif

static int load_mods(const char* dir, struct modmap* maps, int maxmaps) {
#ifdef _WIN32
    char pattern[1024]; snprintf(pattern,sizeof(pattern), "%s\\*.xfmod", dir);
    struct _finddata_t fd; long h = _findfirst(pattern,&fd); if (h == -1) return 0;
    int count = 0;
    do {
        char path[1024]; snprintf(path,sizeof(path), "%s\\%s", dir, fd.name);
        DEBUG_LOG("load_mods: reading %s\n", path);
        char* buf = read_file_safely(path); if (!buf) { DEBUG_LOG("load_mods: read_file_safely returned NULL for %s\n", path); continue; }
        DEBUG_LOG("load_mods: buf[0..80]=%.80s\n", buf);
        char* p = buf;
        while (*p) {
            /* find lhs quoted string */
            while (*p && *p != '"') p++; if (!*p) break; char* a = p+1; char* q = strchr(a, '"'); if (!q) break; size_t llen = (size_t)(q - a); if (llen >= 127) llen = 127; char lhs[128]; memcpy(lhs, a, llen); lhs[llen]= '\0'; p = q+1;
                DEBUG_LOG("load_mods: found lhs='%s'\n", lhs);
            /* find '=' */ while (*p && *p != '=') p++; if (!*p) break; p++;
            /* find rhs quoted string */ while (*p && *p != '"') p++; if (!*p) break; char* b = p+1; q = strchr(b, '"'); if (!q) break; size_t rlen = (size_t)(q - b); if (rlen >= 127) rlen = 127; char rhs[128]; memcpy(rhs, b, rlen); rhs[rlen] = '\0'; p = q+1;
                DEBUG_LOG("load_mods: found rhs='%s'\n", rhs);
            if (count < maxmaps) { strncpy(maps[count].from,lhs,sizeof(maps[count].from)-1); maps[count].from[sizeof(maps[count].from)-1]='\0'; strncpy(maps[count].to,rhs,sizeof(maps[count].to)-1); maps[count].to[sizeof(maps[count].to)-1]='\0'; count++; }
        }
        free(buf);
    } while (_findnext(h,&fd) == 0);
    _findclose(h);
    return count;
#else
    DIR* d = opendir(dir); if (!d) return 0; struct dirent* e; int count=0;
    while ((e = readdir(d))!=NULL) {
        const char* name = e->d_name; size_t ln = strlen(name);
        if (ln > 6 && strcmp(name+ln-6, ".xfmod")==0) {
            char path[1024]; snprintf(path,sizeof(path), "%s/%s", dir, name);
            DEBUG_LOG("load_mods: reading %s\n", path);
            char* buf = read_file_safely(path); if (!buf) { DEBUG_LOG("load_mods: read_file_safely returned NULL for %s\n", path); continue; }
            DEBUG_LOG("load_mods: buf[0..80]=%.80s\n", buf);
            char* p = buf;
            while (*p) {
                /* find lhs quoted string */
                while (*p && *p != '"') p++; if (!*p) break; char* a = p+1; char* q = strchr(a, '"'); if (!q) break; size_t llen = (size_t)(q - a); if (llen >= 127) llen = 127; char lhs[128]; memcpy(lhs, a, llen); lhs[llen]= '\0'; p = q+1;
                /* find '=' */ while (*p && *p != '=') p++; if (!*p) break; p++;
                /* find rhs quoted string */ while (*p && *p != '"') p++; if (!*p) break; char* b = p+1; q = strchr(b, '"'); if (!q) break; size_t rlen = (size_t)(q - b); if (rlen >= 127) rlen = 127; char rhs[128]; memcpy(rhs, b, rlen); rhs[rlen] = '\0'; p = q+1;
                if (count < maxmaps) { strncpy(maps[count].from,lhs,sizeof(maps[count].from)-1); maps[count].from[sizeof(maps[count].from)-1]='\0'; strncpy(maps[count].to,rhs,sizeof(maps[count].to)-1); maps[count].to[sizeof(maps[count].to)-1]='\0'; count++; }
            }
            free(buf);
        }
    }
    closedir(d);
    return count;
#endif
}

static char* apply_mods(const char* src, struct modmap* maps, int mcount) {
    if (!mcount) return strdup(src);
    size_t cap = strlen(src) + 1024; char* out = malloc(cap); if (!out) return NULL; out[0]='\0'; size_t outlen=0;
    const char* p = src;
    while (*p) {
        if (*p == '"') { /* copy string literal unchanged */
            const char* q = p+1; while (*q && !(*q=='"' && *(q-1)!='\\')) q++; size_t len = (size_t)((*q=='"') ? (q - p + 1) : (q - p));
            if (outlen + len + 1 > cap) { cap = (cap + len + 1)*2; out = realloc(out, cap); }
            memcpy(out+outlen, p, len); outlen += len; out[outlen]='\0'; p += len; continue;
        }
        if (isalpha((unsigned char)*p) || *p=='_') {
            const char* s = p; while (isalnum((unsigned char)*p) || *p=='_') p++; size_t tl = (size_t)(p-s);
            char tok[256]; if (tl >= sizeof(tok)) tl = sizeof(tok)-1; memcpy(tok, s, tl); tok[tl]='\0'; int rep=0;
            for (int i=0;i<mcount;i++) if (strcmp(tok, maps[i].from)==0) {
                size_t rlen = strlen(maps[i].to); if (outlen + rlen + 1 > cap) { cap = (cap + rlen + 1)*2; out=realloc(out,cap); }
                memcpy(out+outlen, maps[i].to, rlen); outlen += rlen; out[outlen]='\0'; rep=1; break;
            }
            if (!rep) { if (outlen + tl + 1 > cap) { cap = (cap + tl + 1)*2; out=realloc(out,cap); } memcpy(out+outlen,tok,tl); outlen += tl; out[outlen]='\0'; }
            continue;
        }
        if (outlen + 2 > cap) { cap *=2; out = realloc(out, cap); }
        out[outlen++] = *p++; out[outlen] = '\0';
    }
    return out;
}

static void write_preamble(FILE* out, int need_time) {
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    if (need_time) fprintf(out, "#include <time.h>\n");
    fprintf(out, "#ifdef _WIN32\n#include <windows.h>\n#include <wchar.h>\n#endif\n\n");
    fprintf(out, "static void print_utf8(const char* s) {\n");
    fprintf(out, "#ifdef _WIN32\n");
    fprintf(out, "    if (!s) return;\n");
    fprintf(out, "    UINT prev = GetConsoleOutputCP();\n");
    fprintf(out, "    SetConsoleOutputCP(CP_UTF8);\n");
    fprintf(out, "    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);\n");
    fprintf(out, "    if (wlen) {\n");
    fprintf(out, "        wchar_t* w = (wchar_t*)malloc((size_t)wlen * sizeof(wchar_t));\n");
    fprintf(out, "        if (w) {\n");
    fprintf(out, "            MultiByteToWideChar(CP_UTF8, 0, s, -1, w, wlen);\n");
    fprintf(out, "            DWORD written; WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), w, wlen-1, &written, NULL);\n");
    fprintf(out, "            wchar_t nl = L'\\n'; WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &nl, 1, &written, NULL);\n");
    fprintf(out, "            free(w);\n");
    fprintf(out, "        }\n");
    fprintf(out, "    }\n");
    fprintf(out, "    SetConsoleOutputCP(prev);\n");
    fprintf(out, "#else\n");
    fprintf(out, "    if (s) { printf(\"%%s\\n\", s); fflush(stdout); }\n");
    fprintf(out, "#endif\n");
    fprintf(out, "}\n\n");
    fprintf(out, "int main(void) {\n");
    if (need_time) fprintf(out, "    srand((unsigned)time(NULL));\n");
}

/* helper to trim */
static char* trimcpy(const char* s, size_t n) { while (n>0 && isspace((unsigned char)s[n-1])) n--; size_t a=0; while (a<n && isspace((unsigned char)s[a])) a++; size_t len = (a>n)?0:(n-a); char* r = malloc(len+1); memcpy(r, s+a, len); r[len]='\0'; return r; }

/* parse generator content like 1...20 or 1...20:2, fill start,end,step, return 1 on ok */
static int parse_range(const char* s, int* out_start, int* out_end, int* out_step) {
    // s expected like 1...20 or 1...20:2
    const char* p = s; while (*p && isspace((unsigned char)*p)) p++;
    char* dots = strstr(p, "..."); if (!dots) return 0;
    char left[64], right[64]; int l=0; const char* q = p; while (q < dots && l+1 < (int)sizeof(left)) left[l++] = *q++; left[l]='\0';
    q = dots + 3; int r=0; while (*q && *q != ':' && r+1 < (int)sizeof(right)) right[r++] = *q++; right[r]='\0';
    int step = 1; if (*q == ':') { q++; step = atoi(q); if (step <= 0) return 0; }
    int a = atoi(left); int b = atoi(right);
    if (b < a) return 0;
    long cnt = (long)((b - a) / step) + 1;
    if (cnt <= 0 || cnt > MAX_RANGE_ELEMENTS) return -1; // too big
    *out_start = a; *out_end = b; *out_step = step; return 1;
}

/* generate a valid C identifier for block+fn */
static void make_fn_name(const char* block, const char* fn, char* out, size_t outsz) {
    snprintf(out, outsz, "%s_%s", block, fn);
    for (size_t i=0;i<strlen(out);i++) if (!isalnum((unsigned char)out[i]) && out[i] != '_') out[i] = '_';
}

/* main simple translator: parse top-level blocks and emit C */
static int parse_and_emit(const char* code, const char* tmpc, const char* modsdir) {
    struct modmap maps[128]; int mcount=0; char* moded = NULL;
    if (modsdir) mcount = load_mods(modsdir, maps, 128);
    DEBUG_LOG("load_mods returned %d\n", mcount);
    if (mcount) moded = apply_mods(code, maps, mcount);
    if (moded) DEBUG_LOG("mods applied, moded len=%zu\n", strlen(moded));
    const char* src = moded ? moded : code;
        DEBUG_LOG("parse_and_emit: src[0..160]=%.160s\n", src);
    FILE* out = fopen(tmpc, "wb"); if (!out) { if (moded) free(moded); return 0; }
    DEBUG_LOG("opened tmpc %s for writing\n", tmpc);
    int need_time = 0;
    // emit preamble later based on need
    // find blocks
    const char* scan = src;
    // we'll collect function bodies and emit all functions then main
    static char functions[65536]; functions[0]='\0'; size_t flen = 0;
    static char main_body[65536]; main_body[0]='\0'; size_t mlen = 0;
    char entry_fn[256]; entry_fn[0] = '\0';
    while (1) {
        const char* ob = strchr(scan, '#'); if (!ob) break;
        // block name
        const char* name_start = ob+1; while (*name_start && isspace((unsigned char)*name_start)) name_start++;
        const char* name_end = name_start; while (*name_end && (isalnum((unsigned char)*name_end) || *name_end == '_' )) name_end++;
        if (name_start == name_end) { scan = ob+1; continue; }
        char blockname[128]; size_t bn = (size_t)(name_end - name_start); if (bn >= sizeof(blockname)) bn = sizeof(blockname)-1; memcpy(blockname, name_start, bn); blockname[bn]='\0';
        const char* oblock = strchr(name_end, '{'); if (!oblock) { scan = name_end; continue; }
        const char* p = oblock + 1; int lvl = 1; const char* start = p;
        while (*p && lvl>0) { if (*p == '{') lvl++; else if (*p == '}') lvl--; p++; }
        if (lvl != 0) break;
        const char* end = p-1;
        // parse inside block
        const char* line = start;
        while (line < end) {
            // read a line or statement until newline
            const char* le = line; while (le < end && *le != '\n') le++;
            // trim
            const char* s = line; while (s < le && isspace((unsigned char)*s)) s++;
            const char* e = le; while (e > s && isspace((unsigned char)e[-1])) e--;
            
            // Handle // comments - only if we have content
            if (s < e) {
                const char* comment_start = strstr(s, "//");
                if (comment_start && comment_start < e) {
                    e = comment_start;
                }
            }
            

            
            size_t llen = (size_t)(e - s);
            if (llen > 0) {
                // handle fn definition: fn name() { ... }
                if (llen >= 3 && strncmp(s, "fn", 2) == 0 && isspace((unsigned char)s[2])) {
                    DEBUG_LOG("found fn line: '%.80s'\n", s);
                    // find fn name and its body
                    const char* fnstart = s+2; while (fnstart < e && isspace((unsigned char)*fnstart)) fnstart++;
                    const char* fnend = fnstart; while (fnend < e && (isalnum((unsigned char)*fnend) || *fnend=='_')) fnend++;
                    char fnname[128]; size_t fnl = (size_t)(fnend - fnstart); if (fnl >= sizeof(fnname)) fnl = sizeof(fnname)-1; memcpy(fnname, fnstart, fnl); fnname[fnl]='\0';
                    // find the function body following this line (look for next '{') in the entire block
                    const char* brace = strchr(line, '{'); if (!brace) { line = le + 1; continue; }
                    const char* q = brace+1; int l=1; const char* bodystart = q;
                    while (q < end && l>0) { if (*q=='{') l++; else if (*q=='}') l--; q++; }
                    const char* bodyend = q-1;
                    // now extract body lines and compile them to C inside function
                    char fname[256]; make_fn_name(blockname, fnname, fname, sizeof(fname));
                    if (entry_fn[0] == '\0') strncpy(entry_fn, fname, sizeof(entry_fn)-1);
                    if (strcmp(fnname, "call") == 0 || strcmp(fnname, "main") == 0 || strcmp(fnname, "Test") == 0 || strcmp(fnname, "you_function_name") == 0) {
                        strncpy(entry_fn, fname, sizeof(entry_fn)-1);
                    }
                    // start function
                    char buf[8192]; snprintf(buf, sizeof(buf), "void %s(void) {\n", fname); size_t bl = strlen(buf); if (flen + bl + 1 < sizeof(functions)) { memcpy(functions+flen, buf, bl); flen += bl; functions[flen]='\0'; }
                    // parse body lines
                    const char* L = bodystart;
                    while (L < bodyend) {
                        const char* NL = L; while (NL < bodyend && *NL != '\n') NL++;
                        const char* ss = L; while (ss < NL && isspace((unsigned char)*ss)) ss++;
                        const char* ee = NL; while (ee > ss && isspace((unsigned char)ee[-1])) ee--;
                        
                        // Handle // comments - only if we have content
                        if (ss < ee) {
                            const char* comment_start = strstr(ss, "//");
                            if (comment_start && comment_start < ee) {
                                ee = comment_start;
                            }
                        }
                        

                        
                        size_t lon = (size_t)(ee - ss);
                        if (lon > 0) {
                            // $block@fn call
                            if (ss[0] == '$') {
                                const char* at = memchr(ss, '@', lon);
                                if (at) {
                                    size_t bnl = (size_t)(at - (ss+1)); char bname[128]; if (bnl >= sizeof(bname)) bnl = sizeof(bname)-1; memcpy(bname, ss+1, bnl); bname[bnl]='\0';
                                    size_t fnl2 = (size_t)(ee - at - 1); if (fnl2 >= sizeof(bname)) fnl2 = sizeof(bname)-1; char f2[128]; memcpy(f2, at+1, fnl2); f2[fnl2]='\0'; char calln[256]; make_fn_name(bname, f2, calln, sizeof(calln)); char cb[512]; snprintf(cb, sizeof(cb), "    %s();\n", calln); size_t cbk = strlen(cb); if (flen + cbk +1 < sizeof(functions)) { memcpy(functions+flen, cb, cbk); flen += cbk; functions[flen]='\0'; }
                                }
                            } else if (lon >= 6 && strncmp(ss, "print(",6)==0) {
                                // extract between quotes (support ascii)
                                const char* qs = strchr(ss, '"');
                                if (qs) {
                                    const char* qe = qs+1; while (*qe && *qe != '"') { if (*qe == '\\' && *(qe+1)) qe+=2; else qe++; }
                                    if (*qe == '"') {
                                        size_t ql = (size_t)(qe - qs - 1);
                                        char* lit = malloc(ql+1); if (lit) {
                                            memcpy(lit, qs+1, ql); lit[ql]='\0';
                                            char* esc = escape_bytes_as_c_string(lit);
                                            if (esc) {
                                                char outln[2048]; snprintf(outln, sizeof(outln), "    print_utf8(\"%s\");\n", esc);
                                                size_t obl=strlen(outln);
                                                if (flen + obl +1 < sizeof(functions)) { memcpy(functions+flen,outln,obl); flen += obl; functions[flen]='\0'; }
                                                free(esc);
                                            }
                                            free(lit);
                                        }
                                    }
                                }
                            } else if (lon > 2 && memchr(ss,'=',lon)) {
                                // simple assignment or generator
                                const char* eq = memchr(ss,'=',lon);
                                const char* idend = eq; while (idend > ss && isspace((unsigned char)idend[-1])) idend--; const char* idstart = idend; while (idstart > ss && (isalnum((unsigned char)idstart[-1]) || idstart[-1]=='_')) idstart--; size_t idl = (size_t)(idend - idstart);
                                char ident[128]; if (idl >= sizeof(ident)) idl = sizeof(ident)-1; memcpy(ident, idstart, idl); ident[idl]='\0';
                                const char* valstart = eq+1; while (valstart < ee && isspace((unsigned char)*valstart)) valstart++; const char* valend = ee; while (valend > valstart && isspace((unsigned char)valend[-1])) valend--; size_t vlen = (size_t)(valend - valstart);
                                char* val = malloc(vlen+1); memcpy(val, valstart, vlen); val[vlen]='\0';
                                // detect generators
                                if (strncmp(val, "random[",7)==0 || strncmp(val, "rnd[",4)==0) {
                                    const char* br = strchr(val,'['); if (br) {
                                        const char* inside = br+1; char tmp[128]; strncpy(tmp, inside, sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0'; char* rb = strchr(tmp,']'); if (rb) *rb='\0'; int a,b,step; char inner[128]; strncpy(inner,tmp,sizeof(inner)-1); inner[sizeof(inner)-1]='\0'; int pr = parse_range(inner,&a,&b,&step);
                                    if (pr == -1) {
                                        DEBUG_LOG("Range too large for %s.%s\n", blockname, ident);
                                        fprintf(stderr, "Error: Range too large for %s.%s (max %d elements)\n", blockname, ident, MAX_RANGE_ELEMENTS); free(val); L = NL+1; continue;
                                    }
                                    need_time = 1;
                                    char linebuf[512]; snprintf(linebuf,sizeof(linebuf), "    int %s = (rand() % (%d - %d + 1)) + %d;\n", ident, b, a, a);
                                    size_t lb = strlen(linebuf); if (flen + lb +1 < sizeof(functions)) { memcpy(functions+flen,linebuf,lb); flen += lb; functions[flen]='\0'; }
                                    }
                                } else if (strncmp(val, "sequential[",11)==0 || strncmp(val, "seq[",4)==0 || strncmp(val, "reciprocal[",11)==0 || strncmp(val, "rcp[",4)==0) {
                                    const char* br = strchr(val,'['); if (!br) { free(val); L = NL+1; continue; } const char* ins = br+1; char tmp2[128]; strncpy(tmp2, ins, sizeof(tmp2)-1); tmp2[sizeof(tmp2)-1]='\0'; char* rc = strchr(tmp2,']'); if (rc) *rc='\0'; int a,b,step; int pr = parse_range(tmp2,&a,&b,&step);
                                    if (pr == -1) { 
                                        DEBUG_LOG("Range too large for %s.%s\n", blockname, ident);
                                        fprintf(stderr, "Error: Range too large for %s.%s (max %d elements)\n", blockname, ident, MAX_RANGE_ELEMENTS); 
                                        free(val); L = NL+1; continue; 
                                    }
                                    if (pr == 0) { 
                                        DEBUG_LOG("Invalid range syntax for %s.%s\n", blockname, ident);
                                        fprintf(stderr, "Error: Invalid range syntax for %s.%s\n", blockname, ident); 
                                        free(val); L = NL+1; continue; 
                                    }
                                    // generate static idx and value
                                    char idxname[128]; snprintf(idxname,sizeof(idxname),"__seq_%s_idx", ident);
                                    char l1[256]; if (strncmp(val, "reciprocal",10)==0 || strncmp(val, "rcp",3)==0) {
                                        snprintf(l1,sizeof(l1),"    static int %s = 0; int %s = %d - (%s++ * %d); if (%s < %d) %s = 0;\n", idxname, ident, b, idxname, step, ident, a, idxname);
                                    } else {
                                        snprintf(l1,sizeof(l1),"    static int %s = 0; int %s = %d + (%s++ * %d); if (%s > %d) %s = 0;\n", idxname, ident, a, idxname, step, ident, b, idxname);
                                    }
                                    size_t l1s=strlen(l1); if (flen + l1s +1 < sizeof(functions)) { memcpy(functions+flen,l1,l1s); flen += l1s; functions[flen]='\0'; }
                                } else {
                                    // default: copy as-is
                                    char linebuf[512]; snprintf(linebuf,sizeof(linebuf),"    int %s = %s;\n", ident, val);
                                    size_t lb = strlen(linebuf); if (flen + lb +1 < sizeof(functions)) { memcpy(functions+flen,linebuf,lb); flen += lb; functions[flen]='\0'; }
                                }
                                free(val);
                            } else if (lon >= 2 && strncmp(ss, "if",2)==0 && isspace((unsigned char)ss[2])) {
                                // if cond { ... }
                                const char* bpos = memchr(ss, '{', lon);
                                const char* conds = ss + 2; while (conds < (ss+lon) && isspace((unsigned char)*conds)) conds++;
                                const char* cend = bpos; while (cend > conds && isspace((unsigned char)cend[-1])) cend--;
                                char cond[256]; size_t cl = (size_t)(cend - conds); if (cl >= sizeof(cond)) cl = sizeof(cond)-1; memcpy(cond, conds, cl); cond[cl]='\0'; char outln[512]; snprintf(outln,sizeof(outln),"    if (%s) {\n", cond); size_t ol = strlen(outln); if (flen + ol +1 < sizeof(functions)) { memcpy(functions+flen,outln,ol); flen += ol; functions[flen]='\0'; }
                            } else if (lon >= 7 && strncmp(ss, "else if",7)==0) {
                                const char* conds = ss + 7; while (conds < ee && isspace((unsigned char)*conds)) conds++;
                                const char* bpos = memchr(conds, '{', (size_t)(ee-conds)); const char* cend = bpos; while (cend > conds && isspace((unsigned char)cend[-1])) cend--;
                                char cond[256]; size_t cl = (size_t)(cend - conds); if (cl >= sizeof(cond)) cl = sizeof(cond)-1; memcpy(cond, conds, cl); cond[cl]='\0'; char outln[512]; snprintf(outln,sizeof(outln),"    else if (%s) {\n", cond); size_t ol = strlen(outln); if (flen + ol +1 < sizeof(functions)) { memcpy(functions+flen,outln,ol); flen += ol; functions[flen]='\0'; }
                            } else if (lon >= 4 && strncmp(ss, "else",4)==0) {
                                char outln[64]; snprintf(outln,sizeof(outln),"    else {\n"); size_t ol=strlen(outln); if (flen + ol +1 < sizeof(functions)) { memcpy(functions+flen,outln,ol); flen += ol; functions[flen]='\0'; }
                            }
                        }
                        L = NL + 1;
                    }
                    // close function
                    const char* fclos = "}\n"; size_t fc = strlen(fclos); if (flen + fc +1 < sizeof(functions)) { memcpy(functions+flen,fclos,fc); flen += fc; functions[flen]='\0'; }
                    line = bodyend + 1; continue;
                }
            }
            line = le + 1;
        }
        scan = end + 1;
    }
    // emit preamble and functions and a main that calls entry function if found
    write_preamble(out, 1);
    // emit functions
    if (flen > 0) fprintf(out, "%s\n", functions);
    DEBUG_LOG("emitted %zu bytes of functions\n", flen);
    // emit main body call
    if (entry_fn[0] != '\0') {
        fprintf(out, "    %s();\n", entry_fn);
    }
    fprintf(out, "    return 0;\n}\n");
    fclose(out);
    DEBUG_LOG("closed tmpc and returning OK\n");
    if (moded) free(moded);
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) { 
        fprintf(stderr, "Usage: %s <input.xf> [-o output] [--mods-dir <dir>] [--debug] [--keep-temp]\n", argv[0]); 
        return 1; 
    }
    const char* infile = NULL; const char* outfile = NULL; const char* modsdir = "mods";
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i], "-o")==0 && i+1<argc) outfile = argv[++i];
        else if (strcmp(argv[i], "--mods-dir")==0 && i+1<argc) modsdir = argv[++i];
        else if (strcmp(argv[i], "--debug")==0) g_debug = 1;
        else if (strcmp(argv[i], "--keep-temp")==0) g_keep_temp = 1;
        else if (argv[i][0] == '-') { }
        else if (!infile) infile = argv[i];
    }
    if (!infile) { fprintf(stderr, "No input file\n"); return 1; }
    char tmpc[512]; snprintf(tmpc, sizeof(tmpc), "temp_%d.c", (int)GET_PID());
    char outname[512]; if (!outfile) { const char* dot = strrchr(infile, '.'); size_t len = dot ? (size_t)(dot - infile): strlen(infile); if (len >= sizeof(outname)) len = sizeof(outname)-1; memcpy(outname, infile, len); outname[len]='\0'; outfile = outname; }
    DEBUG_LOG("infile='%s' outfile='%s' modsdir='%s'\n", infile, outfile, modsdir ? modsdir : "(null)");
    DEBUG_LOG("calling read_file_safely('%s')\n", infile);
    char* code = read_file_safely(infile); if (!code) { fprintf(stderr, "Cannot read %s\n", infile); return 1; }
    DEBUG_LOG("read_file_safely returned len=%zu\n", strlen(code));
    /* Ensure buffer is UTF-8: if input file is in ANSI/GBK on Windows, convert it. */
    char* ucode = ensure_utf8_buffer(code);
    if (!ucode) { free(code); fprintf(stderr, "Failed to normalize input encoding\n"); return 1; }
    code = ucode;
    // parse and emit C to tmpc
    int ok = parse_and_emit(code, tmpc, modsdir);
    if (!ok) { free(code); return 1; }
    free(code);
    // call gcc
    char cmd[1024]; snprintf(cmd, sizeof(cmd), "gcc -std=c11 -O2 -Wall %s -o %s", tmpc, outfile);
    int rc = system(cmd);
    if (rc != 0) { fprintf(stderr, "gcc failed (rc=%d)\n", rc); return 1; }
    printf("Generated: %s\n", outfile);
    // keep temp C for debugging if requested
    if (!g_keep_temp) {
        remove(tmpc);
    } else {
        printf("Temp C file kept: %s\n", tmpc);
    }
    return 0;
}
