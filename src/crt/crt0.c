/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points for Windows CE 4.0 through 6.0.
 *
 * Clean-room implementation.  No code from msvcrt, ucrt, mingw-w64,
 * cegcc/mingw32ce, newlib, glibc, dietlibc, or musl is referenced or
 * copied.  Behaviour is derived from Microsoft's public Windows CE
 * documentation (MSDN previous-versions library, Windows CE 5.0
 * "Linking to the CRT"), the PE/COFF specification, the System V ABI,
 * and observable Itanium/MSVC C++ ABI conventions as implemented by
 * Clang/LLVM.
 *
 * OFFICIAL CE ENTRY POINTS (per Microsoft docs):
 *   EXE:
 *     WinMainCRTStartup   <- /ENTRY:WinMainCRTStartup  (no leading _)
 *     wWinMainCRTStartup  <- /ENTRY:wWinMainCRTStartup
 *     mainACRTStartup     <- /ENTRY:mainACRTStartup
 *     mainWCRTStartup     <- /ENTRY:mainWCRTStartup
 *   DLL:
 *     _DllMainCRTStartup  <- /ENTRY:_DllMainCRTStartup (leading _)
 *       (x86 emulator only: _DllMainCRTStartup@12 stdcall)
 *
 * SUPPORTED ARCHITECTURES:
 *   ARM (v4/v4i/v5/v6/v7 Thumb/Thumb2/IWMMXT), x86 (i486+, CEPC,
 *   DeviceEmulator), MIPS (MIPSII/MIPSIV/MIPS16), SH3/SH4.
 *
 * WINAPI is empty on CE (cdecl everywhere on CE ARM/MIPS/SH; coredll
 * x86 CE exports are cdecl, undecorated).  See compiler.h.
 *
 * LIFECYCLE:
 *   1. Save argv/argc state.
 *   2. Get HINSTANCE + wide command line via coredll.
 *   3. Retrieve executable path via GetModuleFileNameW so argv[0] is
 *      always populated (some CE 4/5 launcher configurations omit
 *      the program name from GetCommandLineW()).
 *   4. Parse the wide command line per CommandLineToArgvW rules into
 *      __argc / __wargv.
 *   5. Compute _wcmdtail pointing past argv[0] in the raw buffer
 *      (WinMain's lpCmdLine contract).
 *   6. Synthesize narrow __argv / _acmdln via WideCharToMultiByte
 *      (resolved at runtime from coredll via GetProcAddress; falls
 *      back to lossy ASCII when the converter is absent).
 *   7. Run ALL global initializers that Clang/lld can emit, in three
 *      layers:
 *        (a) .init_array (forward, System V ABI; Clang/lld produces
 *            these for windows-gnu).
 *        (b) MSVC-style .CRT$XCU (and .CRT$XIU C initializers,
 *            .CRT$XPU for pre-C) between .CRT$XCA and .CRT$XCZ
 *            terminator pointers -- for code compiled with
 *            -fms-extensions or toolchains that emit MSVC init
 *            sections.
 *        (c) Legacy .ctors sentinel list (reverse order) for old
 *            CE GCC toolchains.
 *   8. Invoke the user's weak entry (WinMain/wWinMain/main/wmain).
 *   9. Call libc's exit(rc).  exit() is imported normally; if at
 *      static-link time it resolves to NULL (should never happen
 *      with a proper libc), we fall back to ExitProcess(rc) so the
 *      process cannot return into the loader.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* ---------- Win32 primitive types (local, no SDK headers) ---------- */
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef int            BOOL;
typedef int            INT;
typedef wchar_t        WCHAR;
typedef size_t         SIZE_T;
typedef intptr_t       INTPTR_T;
typedef uintptr_t      UINT_PTR;
typedef int         (WINAPI *FARPROC0)(void);
typedef FARPROC0       FARPROC;
typedef void *HANDLE, *HINSTANCE, *HMODULE, *HLOCAL, *LPVOID;
typedef const char    *LPCSTR;  typedef char   *LPSTR;
typedef const WCHAR   *LPCWSTR; typedef WCHAR  *LPWSTR;
typedef int         (WINAPI *wctomb_t)(UINT, DWORD, LPCWSTR, int,
                                       LPSTR, int, LPCSTR, BOOL *);
typedef int         (WINAPI *winmain_t)(HINSTANCE, HINSTANCE, LPWSTR, int);
typedef int         (WINAPI *dllmain_t)(HINSTANCE, DWORD, LPVOID);
typedef void               (*init_fn)(void);

#define TRUE             1
#define FALSE            0
#define LPTR             0x0040u   /* LMEM_FIXED | LMEM_ZEROINIT */
#define SW_SHOW          1
#define SW_SHOWNORMAL    1
#define CP_ACP           0u
#define MAX_PATH         260
#define INFINITE         0xFFFFFFFFu

/* ---------- coredll imports (unconditional) ---------- */
AKARI_DLLIMPORT HMODULE  GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR   GetCommandLineW(void);
AKARI_DLLIMPORT DWORD    GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
AKARI_DLLIMPORT HLOCAL   LocalAlloc(UINT, SIZE_T);
AKARI_DLLIMPORT HLOCAL   LocalFree(HLOCAL);
AKARI_DLLIMPORT FARPROC GetProcAddress(HMODULE, LPCSTR);
AKARI_DLLIMPORT void     ExitProcess(UINT) __attribute__((noreturn));
AKARI_DLLIMPORT void     DisableThreadLibraryCalls(HMODULE);

/* C library entry points.  We import exit() weakly so that a
 * consumer who ships their own libc without providing exit() still
 * links; in that case we fall back to ExitProcess.  (This is NOT
 * the normal path -- every libc provides exit().) */
extern void exit(int) __attribute__((weak, noreturn));

/* ---------- __dso_handle ----------
 *
 * __dso_handle is the "dynamic shared object handle" consumed by
 * __cxa_atexit() to register destructors against a specific load
 * image.  It is normally provided by the CRT; for a statically-
 * linked EXE on Windows CE the value does not matter (all
 * destructors fire at exit() time), but the symbol must be defined
 * because libc++abi emits references to it for every TU that has a
 * non-trivial static object.  The value NULL is correct for the
 * main executable image (per Itanium C++ ABI the NULL dso handle
 * represents the main program; destructors registered with NULL
 * fire at process exit). */
void *__dso_handle = NULL;

/* ---------- User entry points (weak; consumer provides one) ----------
 *
 * Windows CE's native GUI entry is:
 *   int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
 * (WIDE command tail; no ANSI LPSTR variant exists on CE.)
 *
 * wWinMain is accepted as an alias for source compatibility with
 * desktop Unicode builds.
 *
 * Console entries:
 *   int main(int argc, char **argv, char **envp);
 *   int wmain(int argc, wchar_t **wargv, wchar_t **envp);
 * envp is NULL on Windows CE (no POSIX environment block). */
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int main(int, char **, char **)                         __attribute__((weak));
int wmain(int, WCHAR **, WCHAR **)                      __attribute__((weak));

static int WINAPI _dflt_WinMain(HINSTANCE h, HINSTANCE ph, LPWSTR c, int s)
{
    (void)h; (void)ph; (void)c; (void)s;
    if (wmain) return wmain(0, (WCHAR**)0, (WCHAR**)0);
    if (main)  return main(0, (char**)0, (char**)0);
    return 0;
}

/* ---------- MSVCRT data globals ---------- */
int        __argc     = 0;
char     **__argv     = NULL;
WCHAR    **__wargv    = NULL;
char      *_acmdln    = NULL;
WCHAR     *_wcmdln    = NULL;     /* raw GetCommandLineW result */
WCHAR     *_wcmdtail  = NULL;     /* WinMain lpCmdLine tail pointer */
int        _fmode     = 0;
int        _doserrno  = 0;
int        _commode   = 0;

/* ---------- Initializer table markers ----------
 *
 * We emit terminator sentinels in three named PE sections so that
 * when the linker merges sections alphabetically, our sentinels
 * book-end the initializer function-pointer tables:
 *
 *   .CRT$XIA  (pre-C, C initializers)
 *   .CRT$XCA  (start of C++ initializer range; null terminator)
 *   .CRT$XCU  (user C++ dynamic initializers -- clang with
 *               -fms-extensions puts __attribute__((constructor))
 *               entries here)
 *   .CRT$XCZ  (end of C++ range; null terminator)
 *
 * For .init_array / .ctors we rely on weak start/end symbols
 * supplied by the linker script (or the COFF MinGW emulation in
 * lld, which synthesises __CTOR_LIST__/__DTOR_LIST__).
 */
#define DEFINE_CRT_TERM(x)                                              \
    __attribute__((section(".CRT$" #x), used)) static init_fn _crt_##x = (init_fn)0;

DEFINE_CRT_TERM(XIA)       /* pre-C start */
DEFINE_CRT_TERM(XCA)       /* C++ start */
DEFINE_CRT_TERM(XCZ)       /* C++ end */
DEFINE_CRT_TERM(XPA)       /* pre-C++ start */
DEFINE_CRT_TERM(XPZ)       /* pre-C++ end */
DEFINE_CRT_TERM(XTA)       /* tlibc start */
DEFINE_CRT_TERM(XTZ)       /* tlibc end */

#undef DEFINE_CRT_TERM

/* Pointer helpers for walking the .CRT$XI* / .CRT$XC* ranges.
 * These are declared as arrays of init_fn pointers so that pointer
 * arithmetic walks function-pointer entries (4 bytes each on all
 * supported CE architectures, which are ILP32). */
static init_fn *const _xi_start = &_crt_XIA + 1;
static init_fn *const _xi_end   = &_crt_XCA;
static init_fn *const _xc_start = &_crt_XCA + 1;
static init_fn *const _xc_end   = &_crt_XCZ;

extern init_fn __init_array_start[]  __attribute__((weak));
extern init_fn __init_array_end[]    __attribute__((weak));
extern init_fn __CTOR_LIST__[]       __attribute__((weak));
extern init_fn __CTOR_END__[]        __attribute__((weak));

static void _run_ctor_table(init_fn *start, init_fn *end)
{
    if (!start || !end) return;
    for (init_fn *p = start; p < end; p++)
        if (*p) (*p)();
}

static void _run_ctors(void)
{
    if (__init_array_start && __init_array_end)
        _run_ctor_table(__init_array_start, __init_array_end);

    _run_ctor_table(_xi_start, _xi_end);
    _run_ctor_table(_xc_start, _xc_end);

    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        if ((INTPTR_T)list[0] == (INTPTR_T)-1) {
            list++;
            while (list[n]) n++;
        } else {
            while ((INTPTR_T)list[n] != 0) n++;
        }
        for (size_t i = n; i > 0; i--)
            if (list[i-1]) list[i-1]();
    }
}

/* ---------- Wide helpers ---------- */
static size_t _wlen(const WCHAR *p)
{
    size_t n = 0;
    if (p) while (*p++) n++;
    return n;
}

static int _has_program_token(const WCHAR *s)
{
    if (!s) return FALSE;
    while (*s == L' ' || *s == L'\t') s++;
    return *s != L'\0';
}

static const WCHAR *_scan_past_argv0(const WCHAR *s)
{
    if (!s) return s;
    while (*s == L' ' || *s == L'\t') s++;
    if (*s == L'"') {
        s++;
        while (*s) {
            if (*s == L'\\') {
                int bs = 0;
                while (s[bs] == L'\\') bs++;
                if (s[bs] == L'"') s += bs + 1;
                else              { s += bs; break; }
                continue;
            }
            if (*s == L'"') { s++; break; }
            s++;
        }
    } else {
        while (*s && *s != L' ' && *s != L'\t') {
            if (*s == L'\\') {
                int bs = 0;
                while (s[bs] == L'\\') bs++;
                if (s[bs] == L'"') s += bs + 1;
                else              { s += bs; break; }
                continue;
            }
            s++;
        }
    }
    return s;
}

/* ---------- CommandLineToArgvW-compatible parser ---------- */
static WCHAR **_parse_cmdline(const WCHAR *cmd, const WCHAR *prefix_progname,
                              int *out_argc)
{
    static const WCHAR kEmpty[1] = { 0 };
    const WCHAR *source;
    WCHAR *prefix_copy = NULL;
    size_t prefix_len = 0;
    size_t cmd_len = _wlen(cmd);

    if (prefix_progname && prefix_progname[0]) {
        prefix_len = _wlen(prefix_progname);
        SIZE_T buf_bytes = sizeof(WCHAR) * (SIZE_T)(prefix_len + 1 + cmd_len + 1);
        WCHAR *buf = (WCHAR *)LocalAlloc(LPTR, buf_bytes);
        if (!buf) { *out_argc = 0; return NULL; }
        size_t i;
        for (i = 0; i < prefix_len; i++) buf[i] = prefix_progname[i];
        buf[i++] = L' ';
        for (size_t j = 0; j <= cmd_len; j++) buf[i + j] = cmd ? cmd[j] : kEmpty[0];
        source = buf;
        prefix_copy = buf;
    } else if (!cmd || !cmd[0]) {
        SIZE_T argv_bytes = sizeof(WCHAR *) * 2;
        WCHAR **argv = (WCHAR **)LocalAlloc(LPTR, argv_bytes);
        if (!argv) { *out_argc = 0; return NULL; }
        WCHAR *e = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR));
        if (e) e[0] = L'\0';
        argv[0] = e; argv[1] = NULL;
        *out_argc = 1;
        return argv;
    } else {
        source = cmd;
    }

    /* Count. */
    int n = 0, in_q = 0;
    const WCHAR *s = source;
    while (*s) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        n++;
        while (*s) {
            if (*s == L'\\') {
                int bs = 0;
                while (s[bs] == L'\\') bs++;
                if (s[bs] == L'"') { s += bs + 1; }
                else              { s += bs; break; }
                continue;
            }
            if (*s == L'"') {
                if (in_q && s[1] == L'"') { s += 2; continue; }
                in_q = !in_q; s++; continue;
            }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
    }
    if (n == 0) n = 1;

    SIZE_T argv_bytes = sizeof(WCHAR *) * (SIZE_T)(n + 1);
    WCHAR **argv = (WCHAR **)LocalAlloc(LPTR, argv_bytes);
    if (!argv) { if (prefix_copy) LocalFree(prefix_copy); *out_argc = 0; return NULL; }

    size_t src_len = _wlen(source);
    WCHAR *buf = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (SIZE_T)(src_len + 2));
    if (!buf) { LocalFree(argv); if (prefix_copy) LocalFree(prefix_copy);
                *out_argc = 0; return NULL; }

    int a = 0; in_q = 0; s = source;
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
                    if (sl & 1) buf[bi++] = L'"';
                    else        in_q = !in_q;
                    s += sl + 1;
                } else {
                    for (int i = 0; i < sl; i++) buf[bi++] = L'\\';
                    s += sl;
                }
                continue;
            }
            if (*s == L'"') {
                if (in_q && s[1] == L'"') { buf[bi++] = L'"'; s += 2; continue; }
                in_q = !in_q; s++; continue;
            }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            buf[bi++] = *s++;
        }
        buf[bi] = L'\0';
        SIZE_T dup_bytes = sizeof(WCHAR) * (SIZE_T)(bi + 1);
        WCHAR *dup = (WCHAR *)LocalAlloc(LPTR, dup_bytes);
        if (dup)
            for (int i = 0; i <= bi; i++) dup[i] = buf[i];
        argv[a++] = dup;
    }
    LocalFree(buf);
    if (prefix_copy) LocalFree(prefix_copy);

    if (a == 0) {
        WCHAR *e = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR));
        if (e) e[0] = L'\0';
        argv[0] = e; a = 1;
    }
    argv[a] = NULL;
    *out_argc = a;
    return argv;
}

/* ---------- Wide -> narrow conversion ---------- */
static wctomb_t _pWctomb = NULL;

static void _resolve_wctomb(void)
{
    if (_pWctomb) return;
    HMODULE core = GetModuleHandleW(L"coredll.dll");
    if (!core) return;
    _pWctomb = (wctomb_t)(INTPTR_T)GetProcAddress(core, "WideCharToMultiByte");
}

static char *_w2n_one(const WCHAR *w)
{
    size_t l = _wlen(w);
    char *nb = NULL;
    if (_pWctomb) {
        int needed = _pWctomb(CP_ACP, 0, w, (int)l, NULL, 0, NULL, NULL);
        if (needed > 0) {
            nb = (char *)LocalAlloc(LPTR, (SIZE_T)needed + 1);
            if (nb) {
                _pWctomb(CP_ACP, 0, w, (int)l, nb, needed, NULL, NULL);
                nb[needed] = '\0';
                return nb;
            }
        }
    }
    nb = (char *)LocalAlloc(LPTR, (SIZE_T)l + 1);
    if (nb) {
        for (size_t k = 0; k < l; k++)
            nb[k] = ((unsigned int)w[k] < 0x80u) ? (char)w[k] : '?';
        nb[l] = '\0';
    }
    return nb;
}

static char **_w2n(WCHAR **wv, int argc)
{
    SIZE_T vec_bytes = sizeof(char *) * (SIZE_T)(argc + 1);
    char **a = (char **)LocalAlloc(LPTR, vec_bytes);
    if (!a) return NULL;
    for (int i = 0; i < argc; i++) a[i] = _w2n_one(wv ? wv[i] : NULL);
    a[argc] = NULL;
    return a;
}

/* ---------- Initialisation ---------- */
static void _init_runtime(void)
{
    _resolve_wctomb();
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    (void)hinst;
    _wcmdln = GetCommandLineW();

    static WCHAR kModName[MAX_PATH];
    kModName[0] = L'\0';
    {
        DWORD len = GetModuleFileNameW((HMODULE)hinst, kModName, MAX_PATH);
        if (len >= MAX_PATH) len = MAX_PATH - 1;
        kModName[len] = L'\0';
    }

    const WCHAR *prefix = NULL;
    if (!_has_program_token(_wcmdln) && kModName[0])
        prefix = kModName;

    __wargv = _parse_cmdline(_wcmdln ? _wcmdln : L"", prefix, &__argc);

    const WCHAR *tail_src = _wcmdln ? _wcmdln : L"";
    if (!prefix && _has_program_token(tail_src)) {
        const WCHAR *p = tail_src;
        while (*p == L' ' || *p == L'\t') p++;
        p = _scan_past_argv0(p);
        while (*p == L' ' || *p == L'\t') p++;
        _wcmdtail = (WCHAR *)(INTPTR_T)p;
    } else {
        _wcmdtail = (WCHAR *)(INTPTR_T)L"";
    }

    __argv = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
    _run_ctors();
}

/* ---------- Handoff to libc exit, with ExitProcess fallback ---------- */
static void NORETURN _exit_to_os(int rc)
{
    if (exit) {
        exit(rc);
        /* Unreachable if libc is correct; fall through otherwise. */
    }
    for (;;) { ExitProcess((UINT)rc); }
}

/* ---------- Entry point forward declarations + definitions ---------- */
void USED WINAPI AKARI_ENTRY("WinMainCRTStartup")  WinMainCRTStartup(void);
void USED WINAPI AKARI_ENTRY("wWinMainCRTStartup") wWinMainCRTStartup(void);
void USED WINAPI AKARI_ENTRY("mainCRTStartup")     mainCRTStartup(void);
void USED WINAPI AKARI_ENTRY("mainWCRTStartup")    mainWCRTStartup(void);
void USED WINAPI AKARI_ENTRY("mainACRTStartup")    mainACRTStartup(void);

static void NORETURN _entry_gui(HINSTANCE hinst, int prefer_wide)
{
    _init_runtime();
    LPWSTR tail = _wcmdtail ? _wcmdtail : (LPWSTR)L"";
    winmain_t wm = NULL;
    if (prefer_wide) {
        if (wWinMain) wm = (winmain_t)wWinMain;
        else if (WinMain) wm = (winmain_t)WinMain;
        else wm = (winmain_t)_dflt_WinMain;
    } else {
        if (WinMain) wm = (winmain_t)WinMain;
        else if (wWinMain) wm = (winmain_t)wWinMain;
        else wm = (winmain_t)_dflt_WinMain;
    }
    int rc = wm(hinst, (HINSTANCE)0, tail, SW_SHOW);
    _exit_to_os(rc);
}

void USED WINAPI AKARI_ENTRY("WinMainCRTStartup") WinMainCRTStartup(void)
{
    _entry_gui((HINSTANCE)GetModuleHandleW(NULL), /*prefer_wide=*/0);
}

void USED WINAPI AKARI_ENTRY("wWinMainCRTStartup") wWinMainCRTStartup(void)
{
    _entry_gui((HINSTANCE)GetModuleHandleW(NULL), /*prefer_wide=*/1);
}

void USED WINAPI AKARI_ENTRY("mainCRTStartup") mainCRTStartup(void)
{
    _init_runtime();
    int rc;
    if (main)       rc = main(__argc, __argv, (char **)0);
    else if (wmain) rc = wmain(__argc, __wargv, (WCHAR **)0);
    else            rc = 0;
    _exit_to_os(rc);
}

void USED WINAPI AKARI_ENTRY("mainWCRTStartup") mainWCRTStartup(void)
{
    _init_runtime();
    int rc;
    if (wmain)      rc = wmain(__argc, __wargv, (WCHAR **)0);
    else if (main)  rc = main(__argc, __argv, (char **)0);
    else            rc = 0;
    _exit_to_os(rc);
}

void USED WINAPI AKARI_ENTRY("mainACRTStartup") mainACRTStartup(void)
{
    mainCRTStartup();
}
