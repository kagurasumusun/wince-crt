/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points for Windows CE PE/COFF (ARM Thumb).
 *
 * Responsibility: get from PE entry point to user's WinMain/wWinMain/main
 * with CRT initialisation done (TLS errno, atexit, C++ constructors,
 * __argc/__argv/__wargv from GetCommandLineW). No C library functions
 * are called here; memory for argv is allocated directly from
 * coredll!LocalAlloc and freed by ExitProcess.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* ---- Minimal Win32 forward declarations (coredll.dll) ---- */
typedef void          *HINSTANCE;
typedef void          *HMODULE;
typedef void          *HANDLE;
typedef int            BOOL;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef unsigned short WCHAR;
typedef long           LONG;
typedef const char    *LPCSTR;
typedef char          *LPSTR;
typedef const WCHAR   *LPCWSTR;
typedef WCHAR         *LPWSTR;
typedef void          *LPVOID;
typedef intptr_t       INT_PTR;

#define LPTR                 0x0040
#define TLS_OUT_OF_INDEXES   ((DWORD)0xFFFFFFFF)
#define SW_SHOW              1

AKARI_DLLIMPORT HMODULE GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR  GetCommandLineW(void);
AKARI_DLLIMPORT void    ExitProcess(UINT);
AKARI_DLLIMPORT DWORD   TlsAlloc(void);
AKARI_DLLIMPORT LPVOID  TlsGetValue(DWORD);
AKARI_DLLIMPORT BOOL    TlsSetValue(DWORD, LPVOID);
AKARI_DLLIMPORT LPVOID  LocalAlloc(UINT, UINT);
AKARI_DLLIMPORT HANDLE  LocalFree(HANDLE);
AKARI_DLLIMPORT void    DisableThreadLibraryCalls(HMODULE);

/* ---- CRT helpers in other TUs ---- */
void _akari_errno_init(void);
void _akari_atexit_init(void);
void _akari_atexit_fini(void);

/* ---- MSVCRT globals ---- */
extern int        __argc;
extern char     **__argv;
extern WCHAR    **__wargv;
extern char      *_acmdln;
extern WCHAR     *_wcmdln;

/* ---- User entry points (weak; user provides exactly one) ---- */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) __attribute__((weak));
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) __attribute__((weak));
int main(int, char **, char **) __attribute__((weak));

static int WINAPI _dflt_WinMain(HINSTANCE h, HINSTANCE ph, LPSTR c, int s) {
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, NULL, NULL) : 0;
}
static int WINAPI _dflt_wWinMain(HINSTANCE h, HINSTANCE ph, LPWSTR c, int s) {
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, NULL, NULL) : 0;
}

/* ---- .init_array / .ctors ranges ---- */
typedef void (*init_fn)(void);
extern init_fn __init_array_start[] __attribute__((weak));
extern init_fn __init_array_end[]   __attribute__((weak));
extern init_fn __CTOR_LIST__[]      __attribute__((weak));
extern init_fn __CTOR_END__[]       __attribute__((weak));

static void _run_ctors(void)
{
    if (__init_array_start && __init_array_end) {
        size_t n = (size_t)(__init_array_end - __init_array_start);
        for (size_t i = 0; i < n; i++)
            if (__init_array_start[i]) __init_array_start[i]();
    }
    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        if ((intptr_t)list[0] == -1) { list++; while (list[n]) n++; }
        else { while ((intptr_t)list[n] != 0) n++; }
        for (size_t i = n; i > 0; i--)
            if (list[i-1]) list[i-1]();
    }
}

static unsigned wlen(const WCHAR *p) { unsigned n=0; if (p) while (*p++) n++; return n; }

/* CommandLineToArgvW-style parser. Returns heap (LocalAlloc) array. */
static WCHAR **_parse_cmdline(const WCHAR *cmd, int *out_argc)
{
    if (!cmd) { *out_argc = 0; return NULL; }
    int n = 0, in_q = 0;
    const WCHAR *s = cmd;
    while (*s) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        n++;
        while (*s) {
            if (*s == L'\\') {
                int bs = 0;
                while (s[bs] == L'\\') bs++;
                if (s[bs] == L'"') { s += bs + 1; } else { s += bs; break; }
                continue;
            }
            if (*s == L'"') { in_q = !in_q; s++; continue; }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
    }
    if (n == 0) n = 1;
    WCHAR **argv = (WCHAR**)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR*) * (unsigned)(n+1)));
    if (!argv) { *out_argc = 0; return NULL; }

    unsigned clen = wlen(cmd);
    WCHAR *buf = (WCHAR*)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR) * (clen + 2)));
    if (!buf) { *out_argc = 0; return NULL; }

    int a = 0; in_q = 0; s = cmd;
    while (*s && a < n) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        int bi = 0;
        while (*s) {
            if (*s == L'\\') {
                int sl = 0;
                while (s[sl] == L'\\') sl++;
                if (s[sl] == L'"') {
                    int keep = sl / 2;
                    for (int i = 0; i < keep; i++) buf[bi++] = L'\\';
                    if (sl & 1) buf[bi++] = L'"'; else in_q = !in_q;
                    s += sl + 1;
                } else {
                    for (int i = 0; i < sl; i++) buf[bi++] = L'\\';
                    s += sl;
                }
                continue;
            }
            if (*s == L'"') { in_q = !in_q; s++; continue; }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            buf[bi++] = *s++;
        }
        buf[bi] = L'\0';
        WCHAR *dup = (WCHAR*)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR) * (unsigned)(bi+1)));
        if (dup) { for (int i = 0; i <= bi; i++) dup[i] = buf[i]; }
        argv[a++] = dup;
    }
    LocalFree(buf);
    if (a == 0) {
        argv[0] = (WCHAR*)LocalAlloc(LPTR, sizeof(WCHAR));
        if (argv[0]) argv[0][0] = L'\0';
        a = 1;
    }
    argv[a] = NULL;
    *out_argc = a;
    return argv;
}

static char **_w2n(WCHAR **wv, int argc)
{
    char **a = (char**)LocalAlloc(LPTR, (UINT)(sizeof(char*) * (unsigned)(argc+1)));
    if (!a) return NULL;
    for (int i = 0; i < argc; i++) {
        const WCHAR *w = wv ? wv[i] : NULL;
        unsigned l = wlen(w);
        char *nb = (char*)LocalAlloc(LPTR, l+1);
        if (nb && w) {
            for (unsigned k = 0; k < l; k++) nb[k] = (w[k] < 0x80) ? (char)w[k] : '?';
            nb[l] = '\0';
        } else if (nb) nb[0] = '\0';
        a[i] = nb;
    }
    a[argc] = NULL;
    return a;
}

static void _parse_args(const WCHAR *cmd)
{
    _wcmdln = (WCHAR*)cmd;
    __wargv = _parse_cmdline(cmd, &__argc);
    __argv  = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
}

static void _init_runtime(void)
{
    _akari_errno_init();
    _akari_atexit_init();
    _parse_args(GetCommandLineW());
    _run_ctors();
}

static void NORETURN _exit_to_os(int code) { ExitProcess((UINT)code); for(;;){} }

void NORETURN _akari_cexit(int code)
{
    _akari_atexit_fini();
    _exit_to_os(code);
}

static void NORETURN _entry_winmain(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _init_runtime();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPSTR,int) =
        WinMain ? WinMain : _dflt_WinMain;
    char *tail = "";
    if (__argv && __argc > 1 && __argv[1]) tail = __argv[1];
    int r = wm(hinst, NULL, tail, SW_SHOW);
    _akari_cexit(r);
}
static void NORETURN _entry_wwinmain(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _init_runtime();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPWSTR,int) =
        wWinMain ? (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))wWinMain
                 : (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))_dflt_wWinMain;
    WCHAR *wt = L"";
    if (__wargv && __argc > 1 && __wargv[1]) wt = __wargv[1];
    int r = wm(hinst, NULL, wt, SW_SHOW);
    _akari_cexit(r);
}
static void NORETURN _entry_main(void)
{
    _init_runtime();
    int r = main ? main(__argc, __argv, NULL) : 0;
    _akari_cexit(r);
}

/* PE entry points */
void WINAPI WinMainCRTStartup(void)  { _entry_winmain(); }
void WINAPI wWinMainCRTStartup(void) { _entry_wwinmain(); }
void WINAPI mainCRTStartup(void)     { _entry_main(); }
