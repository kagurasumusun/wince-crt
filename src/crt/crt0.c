/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points (WinMainCRTStartup, wWinMainCRTStartup,
 * mainCRTStartup). Win32 types are forward-declared locally; this CRT
 * does NOT ship a Windows SDK.
 *
 * The only job of this file:
 *   1. Enter from PE entry (no arguments, no return).
 *   2. Parse GetCommandLineW() into __argc / __argv / __wargv / _acmdln.
 *   3. Run C++ constructors recorded in .init_array / .ctors.
 *   4. Call the user's WinMain / wWinMain / main (weak; one is provided).
 *   5. Call exit(return_code). exit() is provided by coredll.dll (or
 *      whichever C library is linked in); it runs atexit handlers
 *      and terminates via ExitProcess.
 *
 * Symbols the consumer's libc / coredll must provide:
 *   exit(), malloc(), free(), LocalAlloc/LocalFree,
 *   GetModuleHandleW, GetCommandLineW, ExitProcess, DisableThreadLibraryCalls.
 *
 * MSVCRT data symbols (__argc, __argv, __wargv, _acmdln, _fmode, _doserrno)
 * are DEFINED here because, despite coredll exporting most C functions,
 * it does NOT export these per-process data globals -- every Windows C
 * runtime (msvcrt / mingwrt / ucrt / this CRT) defines them.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* ---------- forward-declared Win32 / coredll types ---------- */
typedef void *HINSTANCE, *HMODULE, *HANDLE, *LPVOID;
typedef int BOOL;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned short WCHAR;
typedef unsigned long size_t_local;
typedef const char *LPCSTR;  typedef char *LPSTR;
typedef const WCHAR *LPCWSTR; typedef WCHAR *LPWSTR;

#define LPTR               0x0040
#define SW_SHOW            1

AKARI_DLLIMPORT HMODULE GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR  GetCommandLineW(void);
AKARI_DLLIMPORT HANDLE  LocalAlloc(UINT, UINT);
AKARI_DLLIMPORT HANDLE  LocalFree(HANDLE);
AKARI_DLLIMPORT void    DisableThreadLibraryCalls(HMODULE);
/* exit(), malloc(), free() are provided by whichever C library the
 * consumer links against -- static or from coredll.dll. We declare
 * them as ordinary externs (no dllimport) so they resolve from any
 * kind of definition. */
extern void exit(int) __attribute__((noreturn));
extern void *malloc(unsigned long);
extern void  free(void *);

/* ---------- User entry points (weak; consumer provides exactly one) ---------- */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) __attribute__((weak));
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) __attribute__((weak));
int main(int, char **, char **) __attribute__((weak));

static int WINAPI _dflt_WinMain(HINSTANCE h, HINSTANCE ph, LPSTR c, int s) {
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, (char**)0, (char**)0) : 0;
}
static int WINAPI _dflt_wWinMain(HINSTANCE h, HINSTANCE ph, LPWSTR c, int s) {
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, (char**)0, (char**)0) : 0;
}

/* ---------- MSVCRT-visible data globals defined by this CRT ---------- */
int        __argc = 0;
char     **__argv = (void*)0;
WCHAR    **__wargv = (void*)0;
char      *_acmdln = (void*)0;
WCHAR     *_wcmdln = (void*)0;
int        _fmode  = 0;
int        _doserrno = 0;
int        _commode = 0;
unsigned long _akari_sys_blocksize = 4096;

/* ---------- .init_array / .ctors ---------- */
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
        for (size_t i = n; i > 0; i--) if (list[i-1]) list[i-1]();
    }
}

/* ---------- CommandLineToArgvW parser (wchar_t) ---------- */
static unsigned wlen(const WCHAR *p) { unsigned n=0; if (p) while (*p++) n++; return n; }

static WCHAR **_parse_cmdline(const WCHAR *cmd, int *out_argc)
{
    if (!cmd) { *out_argc = 0; return (WCHAR**)0; }
    int n = 0, in_q = 0;
    const WCHAR *s = cmd;
    while (*s) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        n++;
        while (*s) {
            if (*s == L'\\') {
                int bs = 0; while (s[bs] == L'\\') bs++;
                if (s[bs] == L'"') { s += bs+1; } else { s += bs; break; }
                continue;
            }
            if (*s == L'"') { in_q = !in_q; s++; continue; }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
    }
    if (n == 0) n = 1;
    WCHAR **argv = (WCHAR**)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR*) * (unsigned)(n+1)));
    if (!argv) { *out_argc = 0; return (WCHAR**)0; }
    unsigned clen = wlen(cmd);
    WCHAR *buf = (WCHAR*)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR)*(clen+2)));
    if (!buf) { *out_argc = 0; return (WCHAR**)0; }
    int a = 0; in_q = 0; s = cmd;
    while (*s && a < n) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        int bi = 0;
        while (*s) {
            if (*s == L'\\') {
                int sl = 0; while (s[sl] == L'\\') sl++;
                if (s[sl] == L'"') {
                    int keep = sl/2;
                    for (int i = 0; i < keep; i++) buf[bi++] = L'\\';
                    if (sl & 1) buf[bi++] = L'"'; else in_q = !in_q;
                    s += sl+1;
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
        WCHAR *dup = (WCHAR*)LocalAlloc(LPTR, (UINT)(sizeof(WCHAR)*(unsigned)(bi+1)));
        if (dup) { for (int i = 0; i <= bi; i++) dup[i] = buf[i]; }
        argv[a++] = dup;
    }
    LocalFree(buf);
    if (a == 0) {
        argv[0] = (WCHAR*)LocalAlloc(LPTR, sizeof(WCHAR));
        if (argv[0]) argv[0][0] = L'\0';
        a = 1;
    }
    argv[a] = (WCHAR*)0;
    *out_argc = a;
    return argv;
}

static char **_w2n(WCHAR **wv, int argc)
{
    char **a = (char**)LocalAlloc(LPTR, (UINT)(sizeof(char*) * (unsigned)(argc+1)));
    if (!a) return (char**)0;
    for (int i = 0; i < argc; i++) {
        const WCHAR *w = wv ? wv[i] : (const WCHAR*)0;
        unsigned l = wlen(w);
        char *nb = (char*)LocalAlloc(LPTR, l+1);
        if (nb && w) {
            for (unsigned k = 0; k < l; k++) nb[k] = (w[k] < 0x80) ? (char)w[k] : '?';
            nb[l] = '\0';
        } else if (nb) nb[0] = '\0';
        a[i] = nb;
    }
    a[argc] = (char*)0;
    return a;
}

/* ---------- Common runtime init ---------- */
static void _init_runtime(void)
{
    _wcmdln = GetCommandLineW();
    __wargv = _parse_cmdline(_wcmdln, &__argc);
    __argv  = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
    _run_ctors();
}

/* ---------- PE entry points ---------- */
void WINAPI WinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW((const WCHAR*)0);
    _init_runtime();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPSTR,int) =
        WinMain ? WinMain : _dflt_WinMain;
    static char empty_tail[] = "";
    char *tail = empty_tail;
    if (__argv && __argc > 1 && __argv[1]) tail = __argv[1];
    int r = wm(hinst, (HINSTANCE)0, tail, SW_SHOW);
    exit(r);
    for (;;) { }
}

void WINAPI wWinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW((const WCHAR*)0);
    _init_runtime();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPWSTR,int) =
        wWinMain ? (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))wWinMain
                 : (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))_dflt_wWinMain;
    static WCHAR empty_wtail[] = { 0 };
    WCHAR *wt = empty_wtail;
    if (__wargv && __argc > 1 && __wargv[1]) wt = __wargv[1];
    int r = wm(hinst, (HINSTANCE)0, wt, SW_SHOW);
    exit(r);
    for (;;) { }
}

void WINAPI mainCRTStartup(void)
{
    _init_runtime();
    int r = main ? main(__argc, __argv, (char**)0) : 0;
    exit(r);
    for (;;) { }
}
