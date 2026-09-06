/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points: WinMainCRTStartup, wWinMainCRTStartup,
 *            mainCRTStartup.
 *
 * After the loader transfers control here we:
 *  1. Initialize TLS / errno / atexit tables.
 *  2. Call constructors from .init_array / .ctors.
 *  3. Retrieve HINSTANCE / command line and call the user entry point
 *     (WinMain, wWinMain, or main).
 *  4. Call exit() with the return value, which runs atexit handlers and
 *     terminates the process via ExitProcess.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <akari/windef.h>
#include <akari/winnt.h>
#include <akari/compiler.h>

/* Forward declarations for init helpers defined in other translation units */
void _akari_errno_init(void);
void _akari_atexit_register(void);
void _akari_stdio_init(void);

/* Defined in misc/globals.c */
extern int      __argc;
extern char   **__argv;
extern wchar_t **__wargv;
extern char    *_acmdln;
extern wchar_t *_wcmdln;

/* User entry points (weak; one of them is provided by user code). */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) __attribute__((weak));
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) __attribute__((weak));
int main(int argc, char **argv, char **envp) __attribute__((weak));

/*
 * Minimal CommandLineToArgvW-style parser.
 * Parses the Unicode command line per the standard Microsoft C runtime
 * rules: whitespace separates arguments; double quotes group; backslashes
 * escape only when immediately preceding a quote.  Sets __argc, __wargv;
 * caller passes allocated arrays; we heap-allocate storage here.
 */
static wchar_t **_akari_parse_cmdline(const wchar_t *cmd, int *out_argc)
{
    if (!cmd) { *out_argc = 0; return NULL; }
    /* Count args first to size the pointer array. */
    int n = 0, in_q = 0;
    const wchar_t *s = cmd;
    while (*s) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        n++;
        while (*s) {
            if (*s == L'\\') { s++; if (*s) s++; continue; }
            if (*s == L'"')  { in_q = !in_q; s++; continue; }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
    }
    if (n == 0) n = 1;
    wchar_t **argv = (wchar_t**)malloc(sizeof(wchar_t*) * (n + 1));
    if (!argv) { *out_argc = 0; return NULL; }

    /* Measure & copy each arg */
    wchar_t *buf = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(cmd) + 2));
    if (!buf) { free(argv); *out_argc = 0; return NULL; }

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
                    if (sl & 1) buf[bi++] = L'"'; else { in_q = !in_q; }
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
        wchar_t *dup = (wchar_t*)malloc(sizeof(wchar_t) * (bi + 1));
        if (dup) { memcpy(dup, buf, sizeof(wchar_t)*(bi+1)); }
        argv[a++] = dup;
    }
    /* Ensure argv[0] exists */
    if (a == 0) {
        argv[0] = (wchar_t*)malloc(sizeof(wchar_t));
        if (argv[0]) argv[0][0] = L'\0';
        a = 1;
    }
    argv[a] = NULL;
    free(buf);
    *out_argc = a;
    return argv;
}

/* Convert wide argv to narrow using the current CP_ACP mapping (we simply
 * cast BMP chars; non-BMP becomes '?', which is acceptable for CE). */
static char **_akari_wide_to_narrow(wchar_t **wargv, int argc)
{
    char **a = (char**)malloc(sizeof(char*) * (argc + 1));
    if (!a) return NULL;
    for (int i = 0; i < argc; i++) {
        const wchar_t *w = wargv ? wargv[i] : NULL;
        size_t l = w ? wcslen(w) : 0;
        char *nb = (char*)malloc(l + 1);
        if (nb && w) {
            for (size_t k = 0; k < l; k++) nb[k] = (w[k] < 0x80) ? (char)w[k] : '?';
            nb[l] = '\0';
        } else if (nb) { nb[0] = '\0'; }
        a[i] = nb;
    }
    a[argc] = NULL;
    return a;
}

int WINAPI _akari_default_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                  LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    if (main) return main(0, NULL, NULL);
    return 0;
}

int WINAPI _akari_default_wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                   LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    if (main) return main(0, NULL, NULL);
    return 0;
}

/* If user didn't provide WinMain/wWinMain, our defaults are used. */
static int WINAPI (*_winmain_ptr)(HINSTANCE,HINSTANCE,LPSTR,int) = _akari_default_WinMain;
static int WINAPI (*_wwinmain_ptr)(HINSTANCE,HINSTANCE,LPWSTR,int) = _akari_default_wWinMain;

/* Invoke constructors in .init_array (preferred) and .ctors (legacy) */
typedef void (*init_fn)(void);

/* These symbols are provided by the linker script. */
extern init_fn __init_array_start[] __attribute__((weak));
extern init_fn __init_array_end[]   __attribute__((weak));
extern init_fn __CTOR_LIST__[]      __attribute__((weak));
extern init_fn __CTOR_END__[]       __attribute__((weak));

static void _akari_run_ctors(void)
{
    if (__init_array_start && __init_array_end) {
        size_t n = (size_t)(__init_array_end - __init_array_start);
        size_t i;
        for (i = 0; i < n; i++) {
            if (__init_array_start[i]) __init_array_start[i]();
        }
    }
    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        if ((intptr_t)list[0] == -1) {
            list++;
            while (list[n]) n++;
        } else {
            while ((intptr_t)list[n] != 0) n++;
        }
        {
            size_t i;
            for (i = n; i > 0; i--) { /* constructors run reverse order */
                if (list[i-1]) list[i-1]();
            }
        }
    }
}

static void _akari_parse_and_set_args(const wchar_t *cmd)
{
    _wcmdln = (wchar_t*)cmd;
    __wargv = _akari_parse_cmdline(cmd, &__argc);
    __argv  = _akari_wide_to_narrow(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
}

static void _akari_entry_winmain(void)
{
    int ret;
    HINSTANCE hinst = GetModuleHandleW(NULL);
    LPWSTR    cmd   = GetCommandLineW();
    /* Initialize runtime */
    _akari_errno_init();
    _akari_atexit_register();
    _akari_stdio_init();
    _akari_parse_and_set_args(cmd);
    _akari_run_ctors();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPSTR,int) = WinMain ? WinMain : _akari_default_WinMain;
    /* Pass narrow command tail skipping argv[0] */
    char *tail = "";
    if (__argv && __argc > 1 && __argv[1]) tail = __argv[1];
    ret = wm(hinst, NULL, tail, SW_SHOW);
    exit(ret);
}

static void _akari_entry_wwinmain(void)
{
    int ret;
    HINSTANCE hinst = GetModuleHandleW(NULL);
    LPWSTR    cmd   = GetCommandLineW();
    _akari_errno_init();
    _akari_atexit_register();
    _akari_stdio_init();
    _akari_parse_and_set_args(cmd);
    _akari_run_ctors();
    int WINAPI (*wm)(HINSTANCE,HINSTANCE,LPWSTR,int) =
        wWinMain ? (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))wWinMain
                 : (int WINAPI (*)(HINSTANCE,HINSTANCE,LPWSTR,int))_akari_default_wWinMain;
    wchar_t *wtail = L"";
    if (__wargv && __argc > 1 && __wargv[1]) wtail = __wargv[1];
    ret = wm(hinst, NULL, wtail, SW_SHOW);
    exit(ret);
}

static void _akari_entry_main(void)
{
    int ret;
    _akari_errno_init();
    _akari_atexit_register();
    _akari_stdio_init();
    _akari_parse_and_set_args(GetCommandLineW());
    _akari_run_ctors();
    ret = main ? main(__argc, __argv, _environ) : 0;
    exit(ret);
}

/*
 * These are the PE entry points. They take no arguments and never return.
 * Only one is selected by the linker /ENTRY flag.
 */
void WINAPI WinMainCRTStartup(void)  { _akari_entry_winmain(); }
void WINAPI wWinMainCRTStartup(void) { _akari_entry_wwinmain(); }
void WINAPI mainCRTStartup(void)     { _akari_entry_main(); }

/* Unused local silenced */
static void *_akari_unused[] = { (void*)_winmain_ptr, (void*)_wwinmain_ptr };
