/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- EXE entry points for Windows CE (WinMainCRTStartup,
 * wWinMainCRTStartup, mainCRTStartup, mainWCRTStartup, mainACRTStartup).
 *
 * Clean-room implementation.  No code from msvcrt, mingw-w64, cegcc,
 * newlib, or any other CRT was copied; behaviour is derived solely
 * from Microsoft's public Win32 / Windows CE documentation and from
 * the ISO C / Itanium C++ ABI specifications.
 *
 * Lifecycle performed by every entry point:
 *   1. Get HINSTANCE from GetModuleHandleW(NULL).
 *   2. Retrieve the raw wide command line from GetCommandLineW().
 *   3. Parse the command line per CommandLineToArgvW rules (whitespace,
 *      double quotes, backslash 2N/2N+1 escapes, ""-inside-quotes ->
 *      literal '"') into __argc / __wargv.  argv[0] is the program
 *      name token; subsequent argv entries are the whitespace-separated
 *      arguments.
 *   4. Compute _wcmdln_tail -- the pointer into the raw command line
 *      at the start of the arguments after the program name.  This is
 *      what is passed as lpCmdLine to WinMain / wWinMain (Microsoft
 *      convention: lpCmdLine points to the command-line tail
 *      INCLUDING leading whitespace between argv[0] and argv[1], not
 *      to a re-joined or re-allocated copy of argv[1..]).
 *   5. Synthesize narrow __argv / _acmdln from __wargv using
 *      WideCharToMultiByte (CP_ACP) when available from coredll;
 *      fall back to a lossy 7-bit-safe down-conversion if
 *      WideCharToMultiByte cannot be resolved (headless kernel-only
 *      images that lack the codepage converter).
 *   6. Run global constructors from .init_array (forward order), then
 *      the legacy .ctors sentinel list (reverse order, for old GCC-
 *      era ARM-CE toolchains).
 *   7. Invoke the user's weak entry point (WinMain, wWinMain, or main).
 *   8. Call exit(return_code) from the linked C library, which runs
 *      atexit handlers, calls __cxa_finalize (running __cxa_atexit-
 *      registered C++ destructors), flushes stdio, and finally
 *      invokes ExitProcess.
 *
 * envp (third argument to main) is always NULL on Windows CE: CE has
 * no POSIX environment block, no environ global, no
 * GetEnvironmentStrings/SetEnvironmentVariable API.  The third arg is
 * accepted in the main() prototype for source compatibility only.
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

typedef void *HANDLE, *HINSTANCE, *HMODULE, *HLOCAL, *LPVOID;
typedef const char   *LPCSTR;  typedef char   *LPSTR;
typedef const WCHAR  *LPCWSTR; typedef WCHAR  *LPWSTR;

#define LPTR                 0x0040u
#define SW_SHOW              1
#define CP_ACP               0u        /* default ANSI code page for WideCharToMultiByte */
#define CP_UTF8              65001u    /* not used by default; documented for completeness */

/* coredll imports used unconditionally. */
AKARI_DLLIMPORT HMODULE GetModuleHandleW(LPCWSTR);
AKARI_DLLIMPORT LPWSTR  GetCommandLineW(void);
AKARI_DLLIMPORT HLOCAL  LocalAlloc(UINT, SIZE_T);
AKARI_DLLIMPORT HLOCAL  LocalFree(HLOCAL);

/* WideCharToMultiByte lives in coredll on all standard CE images
 * (kernel included).  We import it as a weak symbol: if a consumer
 * links against a headless kernel that omits the codepage converters
 * the symbol resolves to NULL and we fall back to a lossy path.  The
 * function-pointer indirection is needed because __attribute__((weak))
 * on a dllimport does not produce a NULL import -- instead we declare
 * it as a normal extern weak and resolve the address locally. */
extern int __attribute__((weak))
    WideCharToMultiByte(UINT cp, DWORD flags,
                        LPCWSTR src, int srclen,
                        LPSTR dst, int dstlen,
                        LPCSTR defchar, BOOL *useddef);
typedef int (*wctomb_t)(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, BOOL *);
#define _wctomb_fn  ((wctomb_t)(intptr_t)&WideCharToMultiByte)

/* exit()/malloc()/free() come from the consumer's C library; declared
 * as ordinary externs so they resolve from either static or DLL
 * definitions. */
extern void NORETURN exit(int);
extern void *malloc(SIZE_T);
extern void  free(void *);

/* ---------- User entry points (weak; consumer provides one) ----------
 *
 * Windows CE's native GUI entry is
 *   int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
 * taking a WIDE command tail.  There is no ANSI (LPSTR) WinMain on CE.
 * We also accept wWinMain as an alias for source compatibility with
 * desktop code.  The console entry is main(argc, argv, envp); envp is
 * always NULL on CE (see file header comment).
 */
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)  __attribute__((weak));
int main(int, char **, char **)                         __attribute__((weak));

static int WINAPI _dflt_WinMain(HINSTANCE h, HINSTANCE ph, LPWSTR c, int s)
{
    (void)h; (void)ph; (void)c; (void)s;
    return main ? main(0, (char **)0, (char **)0) : 0;
}

/* ---------- MSVCRT-visible data globals defined by this CRT ---------- */
int        __argc     = 0;
char     **__argv     = NULL;
WCHAR    **__wargv    = NULL;
char      *_acmdln    = NULL;
WCHAR     *_wcmdln    = NULL;   /* full wide command line (GetCommandLineW result) */
WCHAR     *_wcmdtail  = NULL;   /* pointer into _wcmdln just after argv[0] -> lpCmdLine */
int        _fmode     = 0;
int        _doserrno  = 0;
int        _commode   = 0;

/* ---------- Constructor dispatch (.init_array / legacy .ctors) ----------
 *
 * Ordering:
 *   - .init_array entries are emitted in forward order and invoked
 *     in forward order (lowest address to highest), matching the
 *     System V ABI and Clang/lld behaviour on every target.
 *   - Legacy .ctors entries are emitted in REVERSE order under the
 *     (count_prefixed|sentinel) GCC convention, so we invoke them
 *     from the end of the list backwards.  On a pure clang/lld
 *     toolchain __CTOR_LIST__/__CTOR_END__ stay NULL (weak) and
 *     this branch is never taken.
 *
 * Priority: constructors with explicit priority (e.g.
 * __attribute__((constructor(101)))) are already placed in the right
 * position by the compiler; we don't need special handling.
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
        if ((intptr_t)list[0] == (intptr_t)-1) {
            list++;
            while (list[n]) n++;
        } else {
            while ((intptr_t)list[n] != 0) n++;
        }
        for (size_t i = n; i > 0; i--) {
            if (list[i-1]) list[i-1]();
        }
    }
}

/* ---------- Wide-string helpers ---------- */
static size_t _wlen(const WCHAR *p)
{
    size_t n = 0;
    if (p) while (*p++) n++;
    return n;
}

/* ---------- CommandLineToArgvW-compatible parser (wide, clean-room) ----------
 *
 * Implements the documented CommandLineToArgvW rules verbatim:
 *
 *   - Arguments are delimited by whitespace (space or tab).
 *   - A double-quoted string can contain embedded whitespace.
 *   - Backslashes are literal unless immediately preceding a double
 *     quote, in which case the 2N/2N+1 rule applies:
 *       * 2n backslashes followed by " produce n backslashes and
 *         start/end a quoted range;
 *       * 2n+1 backslashes followed by " produce n backslashes and a
 *         literal double-quote character.
 *   - Outside a quoted range, a pair of double quotes ("") is
 *     treated as an escaped literal quote only when it appears
 *     INSIDE what is already a quoted region (otherwise it just
 *     toggles quoting).  This matches the observed behaviour of
 *     CommandLineToArgvW on Windows and Windows CE.
 *
 * Returns the heap-allocated argv array, sets *out_argc, and sets
 * *out_argv0_len to the length of argv[0] in WCHARs so the caller
 * can compute _wcmdtail without scanning the command line twice.
 */
static WCHAR **_parse_cmdline(const WCHAR *cmd, int *out_argc, size_t *out_argv0_len)
{
    if (!cmd) { *out_argc = 0; *out_argv0_len = 0; return NULL; }

    /* First pass: count arguments, also locate the extent of argv[0]
     * in the raw string (for lpCmdLine computation later). */
    int n = 0, in_q = 0;
    const WCHAR *s = cmd;
    size_t argv0_chars = 0;
    while (*s) {
        while (*s == L' ' || *s == L'\t') s++;
        if (!*s) break;
        int first = (n == 0);
        const WCHAR *arg_start = s;
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
                /* "" inside a quoted region -> a single literal " */
                if (in_q && s[1] == L'"') { s += 2; continue; }
                in_q = !in_q; s++; continue;
            }
            if (!in_q && (*s == L' ' || *s == L'\t')) break;
            s++;
        }
        if (first) argv0_chars = (size_t)(s - arg_start);
    }
    if (n == 0) { n = 1; argv0_chars = 0; }

    SIZE_T argv_bytes = sizeof(WCHAR *) * (SIZE_T)(n + 1);
    WCHAR **argv = (WCHAR **)LocalAlloc(LPTR, argv_bytes);
    if (!argv) { *out_argc = 0; *out_argv0_len = 0; return NULL; }

    size_t clen = _wlen(cmd);
    WCHAR *buf = (WCHAR *)LocalAlloc(LPTR, sizeof(WCHAR) * (SIZE_T)(clen + 2));
    if (!buf) { LocalFree(argv); *out_argc = 0; *out_argv0_len = 0; return NULL; }

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
    *out_argv0_len = argv0_chars;
    return argv;
}

/* ---------- Wide -> narrow conversion for __argv / _acmdln ----------
 *
 * We prefer the OS codepage converter WideCharToMultiByte(CP_ACP) so
 * that the narrow argv matches the system's ANSI code page (the same
 * choice every Win32 CRT makes).  On CE images that omit the
 * converter (rare; kernel-only configurations), we fall back to a
 * lossy 7-bit-preserving, non-ASCII-to-'?' conversion.  This is
 * acceptable because narrow argv on a Unicode OS is a best-effort
 * compatibility feature; any code that cares about non-ASCII
 * arguments should walk __wargv instead.
 */
static char *_w2n_one(const WCHAR *w)
{
    size_t l = _wlen(w);
    char *nb = NULL;
    wctomb_t fn = _wctomb_fn;
    if (fn) {
        int needed = fn(CP_ACP, 0, w, (int)l, NULL, 0, NULL, NULL);
        if (needed > 0) {
            nb = (char *)LocalAlloc(LPTR, (SIZE_T)needed + 1);
            if (nb) {
                fn(CP_ACP, 0, w, (int)l, nb, needed, NULL, NULL);
                nb[needed] = '\0';
                return nb;
            }
        }
    }
    /* Fallback: lossy ASCII pass-through, non-ASCII -> '?'. */
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
    for (int i = 0; i < argc; i++) {
        a[i] = _w2n_one(wv ? wv[i] : NULL);
    }
    a[argc] = NULL;
    return a;
}

/* ---------- Common runtime initialisation ---------- */
static void _init_runtime(void)
{
    _wcmdln = GetCommandLineW();
    size_t argv0_len = 0;
    __wargv = _parse_cmdline(_wcmdln, &__argc, &argv0_len);
    (void)argv0_len;
    /* Compute _wcmdtail: pointer into the raw GetCommandLineW()
     * buffer, positioned just after the program-name token (and
     * after any leading whitespace between argv[0] and argv[1]).
     * We cannot rely on __wargv[0] for the offset because
     * _parse_cmdline duplicates each token into a LocalAlloc'd
     * buffer; instead re-scan the raw command line the same way
     * the parser counts argv[0], stopping at the first unquoted
     * whitespace. */
    const WCHAR *p = _wcmdln;
    if (p) {
        while (*p == L' ' || *p == L'\t') p++;
        int in_q = 0;
        if (*p == L'"') { in_q = 1; p++; }
        while (*p) {
            if (*p == L'\\') {
                int bs = 0;
                while (p[bs] == L'\\') bs++;
                if (p[bs] == L'"') p += bs + 1;
                else              { p += bs; break; }
                continue;
            }
            if (*p == L'"') {
                if (in_q && p[1] == L'"') { p += 2; continue; }
                in_q = !in_q; p++;
                if (!in_q) break;
                continue;
            }
            if (!in_q && (*p == L' ' || *p == L'\t')) break;
            p++;
        }
        while (*p == L' ' || *p == L'\t') p++;
    }
    _wcmdtail = (WCHAR *)(intptr_t)p;
    __argv  = _w2n(__wargv, __argc);
    if (__argv && __argc > 0 && __argv[0]) _acmdln = __argv[0];
    _run_ctors();
}

/* ---------- PE entry points (forward-declared for -Wmissing-prototypes) ----------
 *
 * All five entry points are exposed from this TU.  Only one is
 * selected by the linker according to the /ENTRY option (or the
 * lld -Wl,-entry:<name> flag); the others are harmlessly included
 * in the same object but never referenced.  All are marked USED to
 * prevent LTO/--gc-sections from discarding them.
 */
void USED WINAPI WinMainCRTStartup(void);
void USED WINAPI wWinMainCRTStartup(void);
void USED WINAPI mainCRTStartup(void);
void USED WINAPI mainWCRTStartup(void);
void USED WINAPI mainACRTStartup(void);

static void NORETURN _entry_wide_gui(HINSTANCE hinst, int is_wide_winmain)
{
    _init_runtime();
    static const WCHAR k_nul[] = { 0 };
    LPWSTR tail = _wcmdtail ? _wcmdtail : (LPWSTR)k_nul;
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

void USED WINAPI WinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_wide_gui(hinst, /*is_wide_winmain=*/0);
}

void USED WINAPI wWinMainCRTStartup(void)
{
    HINSTANCE hinst = (HINSTANCE)GetModuleHandleW(NULL);
    _entry_wide_gui(hinst, /*is_wide_winmain=*/1);
}

void USED WINAPI mainCRTStartup(void)
{
    _init_runtime();
    /* envp is NULL on Windows CE (no POSIX environment block). */
    int rc = main ? main(__argc, __argv, (char **)0) : 0;
    exit(rc);
    for (;;) { }
}

/* Console-family aliases recognised by Microsoft's linker.
 * mainWCRTStartup = wide console, mainACRTStartup = narrow console.
 * On CE the narrow console entry is still Unicode at the OS level;
 * both just chain to mainCRTStartup since our narrow argv is
 * already synthesised in __argv. */
void USED WINAPI mainWCRTStartup(void) { mainCRTStartup(); }
void USED WINAPI mainACRTStartup(void) { mainCRTStartup(); }
