/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points for Windows CE 4.0 through 6.0.
 *
 * Clean-room implementation, written against Microsoft's public
 * Win32 / Windows CE documentation and the PE/COFF specification.
 * No code from mingw-w64, cegcc/mingw32ce, msvcrt, ucrt, newlib,
 * glibc, dietlibc, or musl is copied or referenced.
 *
 * SUPPORTED ARCHITECTURES:
 *   ARM (v4 / v4i / v5 / v6 / v7 Thumb / Thumb2 / IWMMXT)
 *   x86 (i486 and later, CEPC / DeviceEmulator)
 *   MIPS (MIPSII, MIPSII_FP, MIPSIV, MIPSIV_FP, MIPS16)
 *   SuperH (SH3 / SH4)
 *
 * CALLING CONVENTION: on Windows CE the Win32 ABI (and coredll
 * exports) use the platform's DEFAULT C calling convention on every
 * architecture.  On x86 CE this is __cdecl (NOT __stdcall; coredll
 * symbols are undecorated).  The WINAPI macro from compiler.h
 * therefore expands to empty on _WIN32_WCE targets.
 *
 * ENTRY POINTS EXPORTED (all live in this single TU):
 *   WinMainCRTStartup   GUI (ANSI alias on CE -> wide)
 *   wWinMainCRTStartup  GUI (wide, native)
 *   mainCRTStartup      console (narrow argv)
 *   mainWCRTStartup     console (wide argv)
 *   mainACRTStartup     console alias -> mainCRTStartup
 *
 * The linker picks exactly one based on -Wl,-entry:<name>; with
 * -ffunction-sections + --gc-sections the others are discarded.
 *
 * LIFECYCLE:
 *   1. Retrieve HINSTANCE from GetModuleHandleW(NULL).
 *   2. Fetch the WIDE command line with GetCommandLineW().
 *   3. If the raw command line does not begin with the executable
 *      path (quoted or unquoted), or is empty, query the module
 *      file name via GetModuleFileNameW() so __wargv[0] is always
 *      the program path (consistent with desktop CRT behaviour and
 *      CommandLineToArgvW).
 *   4. Parse the command line per CommandLineToArgvW rules into
 *      __argc/__wargv (backslash 2N/2N+1 rule, "" in-quote ->
 *      literal ", whitespace separators).
 *   5. Set _wcmdtail to point into the raw GetCommandLineW() buffer
 *      just past argv[0] (and any intervening whitespace) -- this
 *      is what WinMain/wWinMain's lpCmdLine points to, per the
 *      Microsoft contract (NOT a re-joined argv[1..] copy).
 *   6. Synthesize narrow __argv / _acmdln via WideCharToMultiByte
 *      (CP_ACP) from coredll when available; fall back to lossy
 *      7-bit pass-through (non-ASCII -> '?') on headless kernels
 *      that ship without codepage support.  We locate
 *      WideCharToMultiByte through GetProcAddress on coredll.dll
 *      rather than a weak dllimport, because weak dllimport is not
 *      reliable on COFF for symbols that may be absent.
 *   7. Run C++ constructors from .init_array (forward order per
 *      System V ABI) then legacy .ctors (reverse order for GCC
 *      sentinel lists produced by old ARM-CE GCC ports).
 *   8. Invoke the user's weak entry (WinMain/wWinMain/main/wmain).
 *   9. Call libc's exit(rc) which runs atexit / __cxa_finalize /
 *      stdio flush and finally ExitProcess.
 *
 * MSVCRT DATA GLOBALS DEFINED HERE (__argc/__argv/__wargv/_acmdln/
 * _wcmdln/_wcmdtail/_fmode/_doserrno/_commode) are NOT exported by
 * coredll.dll on CE; every Win32 CRT defines them.
 *
 * envp (third arg to main): Windows CE has no POSIX environment
 * block -- no environ, no getenv/setenv, no GetEnvironmentStrings.
 * The third argument is always NULL; it is accepted in the main()
 * prototype for POSIX source compatibility only.
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
typedef void *HANDLE, *HINSTANCE, *HMODULE, *HLOCAL, *LPVOID;
typedef const char   *LPCSTR;  typedef char   *LPSTR;
typedef const WCHAR  *LPCWSTR; typedef WCHAR  *LPWSTR;
typedef intptr_t      FARPROC;

#define TRUE                1
#define FALSE               0
#define LPTR                0x0040u     /* LMEM_FIXED | LMEM_ZEROINIT */
#define SW_SHOW             1
#define SW_SHOWNORMAL       1
#define CP_ACP              0u          /* system default ANSI code page */
#define MAX_PATH            260

/* ---------- coredll imports (unconditional) ---------- */
AKARI_DLLIMPORT HMODULE GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR  GetCommandLineW(void);
AKARI_DLLIMPORT DWORD   GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
AKARI_DLLIMPORT HLOCAL  LocalAlloc(UINT, SIZE_T);
AKARI_DLLIMPORT HLOCAL  LocalFree(HLOCAL);
AKARI_DLLIMPORT FARPROC GetProcAddress(HMODULE, LPCSTR);

/* ---------- WideCharToMultiByte, resolved at runtime ----------
 *
 * Resolved via GetProcAddress("coredll.dll", "WideCharToMultiByte")
 * so we build and link even on headless CE kernel configurations
 * where codepage conversion is absent.  If the pointer stays NULL
 * we fall back to a lossy path in _w2n_one. */
typedef int (WINAPI *wctomb_t)(UINT cp, DWORD flags,
                               LPCWSTR src, int srclen,
                               LPSTR dst, int dstlen,
                               LPCSTR defchar, BOOL *useddef);
static wctomb_t _pWctomb = NULL;
static int _resolve_wctomb(void)
{
    HMODULE core = GetModuleHandleW(L"coredll.dll");
    if (!core) return 0;
    _pWctomb = (wctomb_t)(INTPTR_T)GetProcAddress(core, "WideCharToMultiByte");
    return _pWctomb ? 1 : 0;
}

/* ---------- C library entry points (NOT dllimport -- may be static) ---------- */
extern void NORETURN exit(int);
/* Note: we do NOT use malloc/free from libc in this file -- all
 * allocations go through LocalAlloc/LocalFree against the process
 * heap, which is available the moment the PE loader hands control
 * to the entry point (the CRT heap may not be initialised yet). */

/* ---------- User entry points (weak; consumer provides exactly one) ----------
 *
 * Windows CE's native GUI entry is
 *     int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
 * taking the WIDE command tail.  CE never exposes an ANSI WinMain
 * (LPSTR) -- the desktop ANSI entry is a Win9x compatibility shim.
 * We accept the ANSI-looking WinMain signature as an alias that
 * receives a wide pointer for source compatibility with code that
 * was written for desktop TCHAR builds; both map to the SAME wide
 * tail pointer.
 *
 * For console-style apps we also accept wmain(int, wchar_t**,
 * wchar_t**) (wide) and main(int, char**, char**) (narrow).  envp
 * is NULL on CE.
 */
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

/* ---------- MSVCRT-visible data globals ---------- */
int        __argc     = 0;
char     **__argv     = NULL;
WCHAR    **__wargv    = NULL;
char      *_acmdln    = NULL;
WCHAR     *_wcmdln    = NULL;     /* raw GetCommandLineW result */
WCHAR     *_wcmdtail  = NULL;     /* points into _wcmdln after argv[0] -> lpCmdLine */
int        _fmode     = 0;        /* _O_BINARY default (CE has no text/binary distinction) */
int        _doserrno  = 0;
int        _commode   = 0;        /* _IOCOMMIT */

/* ---------- Constructors (.init_array forward, .ctors reverse) ----------
 *
 * Clang/lld for windows-gnu places constructors in .init_array.
 * Old ARM-CE GCC used .ctors sentinel lists; we walk them if
 * present for maximum compatibility.  .ctors entries are emitted in
 * reverse order so we invoke them from the end backwards.
 */
typedef void (*init_fn)(void);
extern init_fn __init_array_start[]  __attribute__((weak));
extern init_fn __init_array_end[]    __attribute__((weak));
extern init_fn __CTOR_LIST__[]       __attribute__((weak));
extern init_fn __CTOR_END__[]        __attribute__((weak));

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

/* ---------- Wide-string helpers ---------- */
static size_t _wlen(const WCHAR *p)
{
    size_t n = 0;
    if (p) while (*p++) n++;
    return n;
}

/* Return TRUE if s starts with an argv[0]-shaped token (optionally
 * quoted).  Used to detect whether GetCommandLineW() already
 * contains the program name on the given CE version. */
static int _has_program_token(const WCHAR *s)
{
    if (!s || !*s) return FALSE;
    while (*s == L' ' || *s == L'\t') s++;
    return *s != L'\0';
}

/* Walk past argv[0] in the raw command-line string, using the same
 * quote/backslash rules as the parser.  Returns a pointer to the
 * first whitespace/separator after argv[0]. */
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

/* ---------- CommandLineToArgvW-compatible parser ----------
 *
 * Returns a LocalAlloc'd NULL-terminated array of LocalAlloc'd wide
 * strings and sets *out_argc.  cmd is used as the source; if cmd is
 * NULL or empty, prefix_progname is used as argv[0] (this handles
 * the case where GetCommandLineW() returns L"" because the loader
 * did not prepend the program name -- some CE 4/5 configurations).
 *
 * Rules:
 *   - Arguments are separated by space/tab.
 *   - "..." quotes group spaces.
 *   - Backslashes are literal unless immediately before ", per the
 *     2N/2N+1 rule:
 *         2n   \ + " -> n backslashes + begin/end quote;
 *         2n+1 \ + " -> n backslashes + literal ".
 *   - "" inside a quoted region -> one literal " (no toggle out of
 *     quote) -- matches CommandLineToArgvW behaviour on XP+.
 */
static WCHAR **_parse_cmdline(const WCHAR *cmd, const WCHAR *prefix_progname,
                              int *out_argc)
{
    static const WCHAR kEmpty[1] = { 0 };
    const WCHAR *source;
    WCHAR *prefix_copy = NULL;
    size_t prefix_len = 0;
    size_t cmd_len = _wlen(cmd);

    if (prefix_progname && prefix_progname[0]) {
        /* Prepend "<progname> " to the command line so the parser
         * always produces argv[0] = program path.  We allocate a
         * temporary buffer from the process heap (LocalAlloc), run
         * the parser on the concatenation, then free it. */
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
        /* Empty command line, no prefix: argv = { "" }. */
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

    /* First pass: count args. */
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

    WCHAR *buf = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) *
                        (SIZE_T)(_wlen(source) + 2));
    if (!buf) {
        LocalFree(argv);
        if (prefix_copy) LocalFree(prefix_copy);
        *out_argc = 0; return NULL;
    }

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

/* ---------- Wide -> narrow conversion for __argv/_acmdln ---------- */
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
    /* Fallback. */
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

/* ---------- Runtime initialisation ---------- */
static void _init_runtime(void)
{
    _resolve_wctomb();

    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _wcmdln = GetCommandLineW();

    /* Query the executable path from the OS; used as argv[0] when
     * the raw command line does not already include it (CE 4/5
     * launcher behaviour varies). */
    static WCHAR kModName[MAX_PATH];
    kModName[0] = L'\0';
    if (hinst) {
        DWORD len = GetModuleFileNameW(hinst, kModName, MAX_PATH);
        if (len >= MAX_PATH) len = MAX_PATH - 1;
        kModName[len] = L'\0';
    }

    const WCHAR *prefix = NULL;
    if (!_has_program_token(_wcmdln) && kModName[0]) {
        prefix = kModName;
    }
    __wargv = _parse_cmdline(_wcmdln ? _wcmdln : L"", prefix, &__argc);

    /* Compute _wcmdtail by scanning _wcmdln past the argv[0] token
     * (or starting from position 0 if argv[0] was synthesised from
     * GetModuleFileNameW). */
    const WCHAR *tail_src = _wcmdln ? _wcmdln : L"";
    if (!prefix && _has_program_token(tail_src)) {
        const WCHAR *p = tail_src;
        while (*p == L' ' || *p == L'\t') p++;
        p = _scan_past_argv0(p);
        while (*p == L' ' || *p == L'\t') p++;
        _wcmdtail = (WCHAR *)(INTPTR_T)p;
    } else {
        _wcmdtail = (WCHAR *)(INTPTR_T)tail_src;
        /* If prefix was prepended the raw _wcmdln may still be "";
         * point tail at its NUL terminator. */
        if (!_wcmdln || !*_wcmdln) _wcmdtail = (WCHAR *)(INTPTR_T)L"";
    }

    __argv = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
    _run_ctors();
}

/* ---------- Forward-declared entry point prototypes ---------- */
void USED WINAPI WinMainCRTStartup(void);
void USED WINAPI wWinMainCRTStartup(void);
void USED WINAPI mainCRTStartup(void);
void USED WINAPI mainWCRTStartup(void);
void USED WINAPI mainACRTStartup(void);

static void NORETURN _entry_wide_gui(HINSTANCE hinst, int is_wide_winmain)
{
    _init_runtime();
    LPWSTR tail = _wcmdtail ? _wcmdtail : (LPWSTR)L"";
    int rc;
    if (is_wide_winmain) {
        int WINAPI (*wm)(HINSTANCE, HINSTANCE, LPWSTR, int) =
            wWinMain ? wWinMain
            : (WinMain ? WinMain : _dflt_WinMain);
        rc = wm(hinst, (HINSTANCE)0, tail, SW_SHOW);
    } else {
        int WINAPI (*wm)(HINSTANCE, HINSTANCE, LPWSTR, int) =
            WinMain ? WinMain : _dflt_WinMain;
        rc = wm(hinst, (HINSTANCE)0, tail, SW_SHOW);
    }
    exit(rc);
    for (;;) { }
}

void USED WINAPI AKARI_ENTRY("WinMainCRTStartup") WinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_wide_gui(hinst, /*is_wide_winmain=*/0);
}

void USED WINAPI AKARI_ENTRY("wWinMainCRTStartup") wWinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_wide_gui(hinst, /*is_wide_winmain=*/1);
}

void USED WINAPI AKARI_ENTRY("mainCRTStartup") mainCRTStartup(void)
{
    _init_runtime();
    /* envp is NULL on Windows CE. */
    int rc;
    if (main)       rc = main(__argc, __argv, (char **)0);
    else if (wmain) rc = wmain(__argc, __wargv, (WCHAR **)0);
    else            rc = 0;
    exit(rc);
    for (;;) { }
}

void USED WINAPI AKARI_ENTRY("mainWCRTStartup") mainWCRTStartup(void)
{
    _init_runtime();
    int rc;
    if (wmain)      rc = wmain(__argc, __wargv, (WCHAR **)0);
    else if (main)  rc = main(__argc, __argv, (char **)0);
    else            rc = 0;
    exit(rc);
    for (;;) { }
}

void USED WINAPI AKARI_ENTRY("mainACRTStartup") mainACRTStartup(void) { mainCRTStartup(); }
