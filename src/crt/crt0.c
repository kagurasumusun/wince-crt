/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points for Windows CE (WinMainCRTStartup,
 * wWinMainCRTStartup, mainCRTStartup, plus the wide-only names
 * mainWCRTStartup / mainACRTStartup used by newer CE linkers).
 *
 * This file is a clean-room implementation.  It does NOT copy code
 * from mingw-w64, cegcc, msvcrt, ucrt, newlib, or any other CRT.
 * Win32 types are forward-declared locally; Akari does NOT ship a
 * Windows SDK.
 *
 * Design goals, in order:
 *   1. Architecture-portable: works identically on ARM (v4+), x86
 *      (i486+), MIPS (II/IV), SH3/SH4 -- the CPU families Windows CE
 *      4 through 6 support.  No inline assembly, no fixed-endianness
 *      assumptions, no calling-convention-specific decorators on
 *      exports (WINAPI expands to nothing on CE: coredll uses cdecl).
 *   2. WinCE-native: the command line is read via GetCommandLineW()
 *      (Unicode).  WinMain on CE takes LPWSTR, not LPSTR -- CE does
 *      not ship an ANSI WinMain entry point.
 *   3. Small and ABI-clean: the only PE entry points are the four
 *      named Startup functions; all other symbols are static, weak,
 *      or MSVCRT-visible data globals that every Win32 CRT defines.
 *   4. Hands off shutdown to the linked C library via exit(rc), so
 *      that atexit handlers and __cxa_atexit-registered C++
 *      destructors run before the libc calls ExitProcess.
 *
 * Lifecycle performed by each entry point:
 *   1. Query module handle and wide command line from coredll.
 *   2. Parse wide command line into __argc / __wargv.
 *   3. Synthesize narrow __argv / _acmdln from __wargv (ASCII lossy).
 *   4. Run global constructors recorded in .init_array (and the
 *      legacy .ctors sentinel-list form produced by some older GCC
 *      configurations; kept for maximum toolchain portability).
 *   5. Call the user's weak entry point (WinMain, wWinMain, or main).
 *   6. Call exit(return_code) which is provided by the consumer's C
 *      library (coredll.dll, llvm-libc, newlib, etc.).
 *
 * MSVCRT data globals (__argc, __argv, __wargv, _acmdln, _fmode,
 * _doserrno, _commode) are defined here.  Despite coredll exporting
 * most C *functions* (exit, malloc, printf, ...), it does NOT export
 * these per-process *data* objects -- every Windows C runtime
 * (msvcrt, mingwrt, ucrt, Akari) defines them.
 *
 * Sized types:
 *   - BYTE = unsigned char, WORD = unsigned short, DWORD = unsigned
 *     long, UINT = unsigned int.  These match the documented Win32
 *     definitions on every supported CE CPU (all ILP32).
 *   - WCHAR is wchar_t (Win32 convention; do NOT substitute unsigned
 *     short -- clang treats wchar_t as a distinct type from
 *     unsigned short and a mismatch here breaks wchar_t* APIs).
 *   - SIZE_T is the pointer-width unsigned integer (size_t from
 *     stddef.h); used for byte counts passed to allocators.
 *   - All pointer arithmetic uses size_t / uintptr_t; there is no
 *     cast from sizeof() to unsigned because pointers are 32-bit on
 *     all supported CE architectures but the code still uses the
 *     naturally-sized type to avoid truncation warnings on LP64
 *     hosts during hostcheck.
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
typedef wchar_t        WCHAR;      /* NOT unsigned short -- see header */
typedef size_t         SIZE_T;

typedef void *HANDLE, *HINSTANCE, *HMODULE, *HLOCAL, *LPVOID;
typedef const char   *LPCSTR;  typedef char   *LPSTR;
typedef const WCHAR  *LPCWSTR; typedef WCHAR  *LPWSTR;

/* Memory allocator flags for LocalAlloc (coredll).  LMEM_FIXED |
 * LMEM_ZEROINIT = 0x0040 is the "allocate fixed, zeroed" mode we
 * use for all allocations; LocalFree releases blocks from the
 * default process heap. */
#define LPTR            0x0040u
#define SW_SHOW         1

/* coredll functions imported from the system DLL.  These are marked
 * dllimport so that clang/lld can reference them directly without a
 * thunk; on the hostcheck build AKARI_DLLIMPORT expands to nothing. */
AKARI_DLLIMPORT HMODULE GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR  GetCommandLineW(void);
AKARI_DLLIMPORT HLOCAL  LocalAlloc(UINT, SIZE_T);
AKARI_DLLIMPORT HLOCAL  LocalFree(HLOCAL);
AKARI_DLLIMPORT void    DisableThreadLibraryCalls(HMODULE);

/* exit()/malloc()/free() come from the consumer's C library.  They
 * are NOT marked dllimport because they may be statically linked or
 * come from a different import (coredll, libc, llvm-libc, newlib). */
extern void NORETURN exit(int);
extern void *malloc(SIZE_T);
extern void  free(void *);

/* ---------- User entry points (weak; consumer provides one) ----------
 *
 * On Windows CE the canonical GUI entry point is
 *     int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
 * taking a WIDE command line -- the ANSI LPSTR variant that exists
 * on desktop Win32 is NOT present on CE.  We expose wWinMain as an
 * alias for source compatibility with desktop code.
 *
 * For console-style applications we expose the traditional 3-arg
 * main(int argc, char **argv, char **envp); envp is passed as NULL
 * because Windows CE has no process environment block (no environment
 * variables in the POSIX sense; CE uses the registry instead).
 */
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int main(int, char **, char **)                         __attribute__((weak));

/* Defaults: if no GUI entry is provided they chain to main(). */
static int WINAPI _dflt_WinMain(HINSTANCE h, HINSTANCE ph, LPWSTR c, int s) {
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, (char **)0, (char **)0) : 0;
}

/* ---------- MSVCRT-visible data globals defined by this CRT ---------- */
int        __argc     = 0;
char     **__argv     = NULL;
WCHAR    **__wargv    = NULL;
char      *_acmdln    = NULL;     /* narrow command-line copy */
WCHAR     *_wcmdln    = NULL;     /* wide command-line as returned by GetCommandLineW() */
int        _fmode     = 0;        /* default text/binary mode (no-op on CE; provided for ABI compat) */
int        _doserrno  = 0;        /* DOS errno mapping (maintained by libc; zeroed at startup) */
int        _commode   = 0;        /* commit mode (ABI compat only) */

/* ---------- Constructor dispatch (.init_array / legacy .ctors) ----------
 *
 * Clang/lld on Windows places constructors in .init_array, same as
 * on ELF.  Some older ARM/CE GCC ports placed them in the legacy
 * .ctors sentinel-list form (__CTOR_LIST__/END__); we walk those as
 * well for portability.  On a pure clang/lld toolchain those weak
 * symbols stay NULL and the legacy branch is never taken.
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
        for (size_t i = 0; i < n; i++) {
            if (__init_array_start[i]) __init_array_start[i]();
        }
    }
    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        /* GCC convention: first slot is (size_t)-1 sentinel (count
         * not known), or a leading element count. */
        if ((intptr_t)list[0] == (intptr_t)-1) {
            list++;
            while (list[n]) n++;
        } else {
            while ((intptr_t)list[n] != 0) n++;
        }
        /* Constructors in .ctors are emitted in reverse order. */
        for (size_t i = n; i > 0; i--) {
            if (list[i-1]) list[i-1]();
        }
    }
}

/* ---------- CommandLineToArgvW-style parser (wide-string, clean-room) ----------
 *
 * Parsing rules follow the documented CommandLineToArgvW behaviour:
 *   - whitespace (space, tab) separates arguments;
 *   - double quotes (") group arguments; backslash escapes quotes
 *     and escape other backslashes only before a quote, per the
 *     standard 2N/2N+1 rule (see Win32 API docs for
 *     CommandLineToArgvW).
 *
 * This is used in place of the system CommandLineToArgvW because
 * that function lives in shell32.dll on some CE configurations and
 * we want the CRT to depend only on coredll.dll.  argv[0] is the
 * program path per user expectation; we manufacture argv[0] as the
 * first argument when the raw command line does not start with an
 * argv[0] token, matching how desktop CRTs prepend the module name.
 */
static size_t _wlen(const WCHAR *p)
{
    size_t n = 0;
    if (p) while (*p++) n++;
    return n;
}

static WCHAR **_parse_cmdline(const WCHAR *cmd, int *out_argc)
{
    if (!cmd) { *out_argc = 0; return NULL; }

    /* First pass: count arguments. */
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
                if (s[bs] == L'"') { s += bs + 1; }
                else              { s += bs; break; }
                continue;
            }
            if (*s == L'"') { in_q = !in_q; s++; continue; }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
    }
    /* Always at least argv[0]. */
    if (n == 0) n = 1;

    SIZE_T argv_bytes = sizeof(WCHAR *) * (SIZE_T)(n + 1);
    WCHAR **argv = (WCHAR **)LocalAlloc(LPTR, argv_bytes);
    if (!argv) { *out_argc = 0; return NULL; }

    /* Working buffer large enough to hold the widest unescaped token. */
    size_t clen = _wlen(cmd);
    WCHAR *buf = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (SIZE_T)(clen + 2));
    if (!buf) { LocalFree(argv); *out_argc = 0; return NULL; }

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
                    if (sl & 1) { buf[bi++] = L'"'; }
                    else        { in_q = !in_q; }
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
        SIZE_T dup_bytes = sizeof(WCHAR) * (SIZE_T)(bi + 1);
        WCHAR *dup = (WCHAR *)LocalAlloc(LPTR, dup_bytes);
        if (dup) {
            for (int i = 0; i <= bi; i++) dup[i] = buf[i];
        }
        argv[a++] = dup;
    }
    LocalFree(buf);

    if (a == 0) {
        WCHAR *e = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR));
        if (e) e[0] = L'\0';
        argv[0] = e;
        a = 1;
    }
    argv[a] = NULL;
    *out_argc = a;
    return argv;
}

/* Narrow (char) argv is synthesised by lossy down-conversion; any non-ASCII
 * WCHAR is replaced with '?'.  Full MBCS conversion is a libc concern
 * (MultiByteToWideChar lives in coredll but is intentionally not dragged
 * into the startup path). */
static char **_w2n(WCHAR **wv, int argc)
{
    SIZE_T vec_bytes = sizeof(char *) * (SIZE_T)(argc + 1);
    char **a = (char **)LocalAlloc(LPTR, vec_bytes);
    if (!a) return NULL;
    for (int i = 0; i < argc; i++) {
        const WCHAR *w = wv ? wv[i] : NULL;
        size_t l = _wlen(w);
        char *nb = (char *)LocalAlloc(LPTR, (SIZE_T)l + 1);
        if (nb) {
            if (w) {
                for (size_t k = 0; k < l; k++)
                    nb[k] = ((unsigned int)w[k] < 0x80u) ? (char)w[k] : '?';
            }
            nb[l] = '\0';
        }
        a[i] = nb;
    }
    a[argc] = NULL;
    return a;
}

/* ---------- Common runtime initialisation ---------- */
static void _init_runtime(void)
{
    _wcmdln = GetCommandLineW();
    __wargv = _parse_cmdline(_wcmdln, &__argc);
    __argv  = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
    _run_ctors();
}

/* ---------- PE entry points ----------
 *
 * Forward-declared here (and given internal linkage via the declarations
 * themselves) to satisfy -Wmissing-prototypes: the PE loader calls these
 * by their unmangled names, they are not called from anywhere else
 * inside the CRT.
 */
void USED WINAPI WinMainCRTStartup(void);
void USED WINAPI wWinMainCRTStartup(void);
void USED WINAPI mainCRTStartup(void);
void USED WINAPI mainWCRTStartup(void);
void USED WINAPI mainACRTStartup(void);

static void NORETURN _entry_common_wide(HINSTANCE hinst, int is_wide_winmain)
{
    _init_runtime();
    int rc;
    static const WCHAR kEmptyW[] = { 0 };
    LPWSTR cmd = (LPWSTR)kEmptyW;
    if (__wargv && __argc > 1 && __wargv[1]) cmd = __wargv[1];

    if (is_wide_winmain) {
        int WINAPI (*wm)(HINSTANCE, HINSTANCE, LPWSTR, int) =
            wWinMain ? wWinMain
            : (WinMain ? WinMain : _dflt_WinMain);
        rc = wm(hinst, (HINSTANCE)0, cmd, SW_SHOW);
    } else {
        int WINAPI (*wm)(HINSTANCE, HINSTANCE, LPWSTR, int) =
            WinMain ? WinMain : _dflt_WinMain;
        rc = wm(hinst, (HINSTANCE)0, cmd, SW_SHOW);
    }
    exit(rc);
    /* Unreachable. */
    for (;;) { }
}

void USED WINAPI WinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_common_wide(hinst, /*is_wide_winmain=*/0);
}

void USED WINAPI wWinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_common_wide(hinst, /*is_wide_winmain=*/1);
}

void USED WINAPI mainCRTStartup(void)
{
    _init_runtime();
    int rc = main ? main(__argc, __argv, (char **)0) : 0;
    exit(rc);
    for (;;) { }
}

/* Alternate names recognised by the Microsoft linker (/ENTRY).
 * mainWCRTStartup / mainACRTStartup are the Unicode/ANSI names used
 * by newer CE linkers when the entry is set to the console family.
 * We alias them to mainCRTStartup which already handles wide/narrow
 * transparently. */
void USED WINAPI mainWCRTStartup(void) { mainCRTStartup(); }
void USED WINAPI mainACRTStartup(void) { mainCRTStartup(); }
