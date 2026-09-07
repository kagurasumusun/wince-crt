/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * runtime.c -- per-module data objects and startup helpers shared by
 * the EXE entry objects (crt0.c) and the DLL entry object (dllcrt.c):
 *
 *   - the MSVCRT-model data globals (__argc/__argv/__wargv/_acmdln/
 *     _wcmdln/_wcmdtail/_fmode/_doserrno/_commode/__dso_handle);
 *   - a command-line parser written to the parsing rules published in
 *     Microsoft's documentation (CommandLineToArgvW and "Parsing C
 *     command-line arguments": whitespace delimiters, quoting,
 *     2n/2n+1 backslash-before-quote handling, "" inside quotes);
 *   - wide->narrow argument conversion via WideCharToMultiByte(CP_ACP)
 *     when the OS image provides it, with a documented lossy fallback;
 *   - the global constructor/destructor runners;
 *   - the coredll.dll import declarations used by the above.
 *
 * Design/clean-room basis:
 *  - Microsoft public documentation: "Linking to the CRT (Windows CE
 *    5.0)", "Run-time Library Behavior (Windows CE 5.0)", "WinMain
 *    (Windows CE 5.0)", CommandLineToArgvW, GetCommandLine(W),
 *    WideCharToMultiByte, LocalAlloc/LocalFree, and the PE/COFF
 *    format specification.
 *  - Observed behavior of the verified toolchain (kagurasumusun/
 *    llvm-project, branch LLVM-WinCE; details and reproduction
 *    commands in README, "Verified toolchain behavior"): both of the
 *    toolchain's linkers -- ld.lld for *-windows-gnu objects and
 *    lld-link in its -wince mode for arm-pc-wince / i386-pc-wince
 *    objects -- synthesise __CTOR_LIST__ / __DTOR_LIST__ arrays from
 *    the .ctors/.dtors sections Clang emits, and order the .CRT$X*
 *    family the way Microsoft documents for its own linker ("CRT
 *    initialization", Microsoft Learn): subsections are combined in
 *    the order of the part after '$', so user entries (.CRT$XCU /
 *    .CRT$XIU) always land between the NULL sentinels below
 *    (.CRT$XCA/.CRT$XCZ and .CRT$XIA/.CRT$XIZ).  The ordering was
 *    verified on linked images with the user object placed both
 *    before and after the CRT object: the sentinel and entry offsets
 *    are identical in the two orders and no other data falls inside
 *    the walked ranges.
 *  - Observed list layout (verified on i686/ARMNT windows-gnu and on
 *    armel/x86 lld-link -wince images, one and two contributing
 *    translation units, strong and weak references): each
 *    .ctors/.dtors list is the concatenation of the per-object
 *    section contents in link order, bracketed by a -1 header and a 0
 *    terminator, and the list symbols point at the -1 header.  The
 *    walkers below nevertheless skip a leading -1 only when it is
 *    actually present at l[0], so they also handle a list that starts
 *    with a real entry.
 *  - Per-object .ctors/.dtors words are stored by Clang in a target-
 *    dependent order (verified with several declarations per object):
 *    *-windows-gnu targets store them in REVERSE source order
 *    (last-declared first), while the WinCE driver's *-pc-wince
 *    targets store them in source order.  The linkers concatenate the
 *    per-object blocks in link order in both cases.  The backward
 *    walk over __CTOR_LIST__ therefore runs each object's entries
 *    from the end of its block (source order on windows-gnu, reverse
 *    source order on *-pc-wince) and the objects in reverse link
 *    order; the forward walk over __DTOR_LIST__ runs destructors in
 *    the exact reverse of the constructor order in both cases,
 *    because the .dtors blocks are stored symmetrically to the .ctors
 *    blocks.  Windows CE documentation does not specify initializer
 *    order across objects; the backward-ctor/forward-dtor scheme with
 *    destructors as the exact LIFO mirror is Akari's documented
 *    design decision (it needs no runtime bookkeeping).
 *
 * This file makes no libc calls: it uses only the coredll imports
 * listed below (C library features such as malloc/atexit belong to
 * the consumer's C library).
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>
#include <akari/internal.h>

/* ------------------------------------------------------------------ */
/* Local types and constants (no SDK headers are used).               */
/* ------------------------------------------------------------------ */

typedef void (*init_fn)(void);

#define LPTR 0x0040u        /* LMEM_FIXED | LMEM_ZEROINIT */
#define CP_ACP 0u
#define MAX_PATH16 260u

/* WideCharToMultiByte is part of the CE core OS, but OEM-built images
 * may cut optional modules; resolve it at runtime and degrade to a
 * documented lossy fallback when it is absent. */
typedef int (*w2m_fn)(akari_dword, akari_dword,
                      const akari_wchar *, int,
                      char *, int,
                      const char *, const int *);

/* ------------------------------------------------------------------ */
/* coredll.dll imports used by this module.  Consumers resolve them
 * through the import library of their platform SDK (coredll.lib).    */
/* ------------------------------------------------------------------ */

/* The coredll import declarations below pin their COFF symbol names
 * with asm labels: coredll export names carry no x86 C-name
 * decoration (the sysroot import libraries define __imp_<name> with
 * the undecorated name for every architecture), so the references
 * must not pick up the target's leading-underscore decoration. */
AKARI_DLLIMPORT akari_handle GetModuleHandleW(const akari_wchar *)
    __asm__("GetModuleHandleW");
AKARI_DLLIMPORT akari_wchar *GetCommandLineW(void)
    __asm__("GetCommandLineW");
AKARI_DLLIMPORT akari_dword GetModuleFileNameW(akari_handle,
                                               akari_wchar *,
                                               akari_dword)
    __asm__("GetModuleFileNameW");
AKARI_DLLIMPORT void *LocalAlloc(akari_dword, size_t)
    __asm__("LocalAlloc");
AKARI_DLLIMPORT void LocalFree(void *) __asm__("LocalFree");
/* coredll exports GetProcAddress only in its W spelling on every CE
 * generation (verified against the CE 4/5/6 import libraries of the
 * toolchain sysroot); SDK headers map GetProcAddress ->
 * GetProcAddressW the same way. */
AKARI_DLLIMPORT void *GetProcAddressW(akari_handle, const akari_wchar *)
    __asm__("GetProcAddressW");

/* ------------------------------------------------------------------ */
/* Per-module data objects (MSVCRT data model; see crt.h).            */
/* ------------------------------------------------------------------ */

int          __argc    = 0;
char       **__argv    = NULL;
akari_wchar **__wargv  = NULL;
char        *_acmdln   = NULL;
akari_wchar *_wcmdln   = NULL;
akari_wchar *_wcmdtail = NULL;
int          _fmode    = 0;
int          _doserrno = 0;
int          _commode  = 0;

/* Dynamic shared object handle: NULL identifies the main executable
 * (Itanium C++ ABI); dllcrt.c re-points it at the DLL's module handle
 * on DLL_PROCESS_ATTACH so module-scoped destructors can key off it.
 * Weak: a consumer that links a C++ support library which defines
 * __dso_handle itself must not collide with this copy. */
WEAK void *__dso_handle = NULL;

/* ------------------------------------------------------------------ */
/* Section bookends for MSVC-style initializer tables.                */
/*                                                                     */
/* MS-style objects place C and C++ initializer function pointers
 * into COFF sections .CRT$XIU / .CRT$XCU and rely on the linker to
 * order the .CRT$X* family by section name; lld merges and sorts that
 * family (verified on i386-pc-wince and arm-pc-wince images with
 * __declspec(allocate(".CRT$XCU"))/.CRT$XIU user entries in both
 * object orders: the merged .CRT is XCA(0), XCU, XCZ(0), XIA(0),
 * XIU, XIZ(0)).  Note that the toolchain's own clang-cl does NOT use
 * these sections for plain C++ static initializers: it emits GNU-style
 * .ctors entries (__GLOBAL__sub_I_*) and registers destructors through
 * atexit (observed on both architectures) -- those run through
 * __CTOR_LIST__ below.  The NULL sentinels below bracket every user
 * entry whose section name sorts between the pairs:
 *
 *     .CRT$XIA ..[user .CRT$XI* C inits].. .CRT$XIZ
 *     .CRT$XCA ..[user .CRT$XC* C++ inits].. .CRT$XCZ
 *
 * Ranges are walked start-exclusive..end-exclusive so the sentinels
 * themselves are never invoked.  Clang's windows-gnu driver does not
 * emit .CRT$X* (it emits .ctors/.dtors, handled below through
 * __CTOR_LIST__/__DTOR_LIST__); every mechanism is a no-op when its
 * tables are empty, which keeps links that mix object styles
 * well-defined.                                                      */
/* ------------------------------------------------------------------ */

#define DEFINE_CRT_TERM(x)                                             \
    USED SECTION(".CRT$" #x) static init_fn _akari_crt_##x =           \
        (init_fn) 0;

DEFINE_CRT_TERM(XIA)
DEFINE_CRT_TERM(XIZ)
DEFINE_CRT_TERM(XCA)
DEFINE_CRT_TERM(XCZ)

#undef DEFINE_CRT_TERM

/* lld (COFF, windows-gnu flavour) defines these from the input
 * .ctors/.dtors sections when they are referenced (weak references
 * included, verified); declared weak so that linking with a
 * different PE linker degrades to "no lists" instead of a hard
 * error. */
extern init_fn __CTOR_LIST__[] WEAK;
extern init_fn __DTOR_LIST__[] WEAK;

static init_fn *const xi_begin = &_akari_crt_XIA + 1;
static init_fn *const xi_end   = &_akari_crt_XIZ;
static init_fn *const xc_begin = &_akari_crt_XCA + 1;
static init_fn *const xc_end   = &_akari_crt_XCZ;

/* ------------------------------------------------------------------ */
/* Small helpers.                                                     */
/* ------------------------------------------------------------------ */

static size_t wcs_len(const akari_wchar *s)
{
    size_t n = 0;
    if (s) {
        while (s[n] != 0) {
            n++;
        }
    }
    return n;
}

/* Append one character to the output pool (counting pass: NULL). */
static void put_wc(akari_wchar *out, size_t *n, akari_wchar c)
{
    if (out) {
        out[*n] = c;
    }
    (*n)++;
}

/* ------------------------------------------------------------------ */
/* Command-line parser                                                */
/*                                                                     */
/* One code path is used for both the counting pass and the
 * materialising pass (vec == NULL => count only), so the two can
 * never disagree.  Rules implemented, from Microsoft documentation
 * (CommandLineToArgvW, "Parsing C command-line arguments"):
 *  - arguments are delimited by space/tab outside quotes;
 *  - inside quotes whitespace is ordinary text;
 *  - 2n backslashes before a quote  -> n backslashes, quote toggles
 *    quote-mode;
 *  - 2n+1 backslashes before a quote -> n backslashes + one literal
 *    quote (quote-mode unchanged);
 *  - backslashes not followed by a quote are literal;
 *  - "" inside a quoted region      -> one literal quote;
 *  - an unterminated quoted region runs to the end of the string;
 *  - a command line that starts with whitespace yields an empty first
 *    argument (CommandLineToArgvW "Important" note).
 *
 * The same rules apply to argv[0] as to every other token -- Akari
 * follows CommandLineToArgvW, whose algorithm covers the program-name
 * token uniformly.  (Microsoft's separate desktop-CRT note that
 * argv[0] is a pathname exempt from the later parsing rules is a
 * property of the desktop startup code, not of CommandLineToArgvW;
 * Windows CE's Unicode command-line model is the CommandLineToArgvW
 * one.)  When the whole command line is an empty string,
 * CommandLineToArgvW's documented behavior -- argv[0] is the full
 * path of the current executable (GetModuleFileNameW) -- is
 * implemented in akari_init_args().
 *
 * Every token is NUL-terminated in the pool.  Returns argc.          */
/* ------------------------------------------------------------------ */

static int parse_pass(const akari_wchar *src,
                      akari_wchar **vec,
                      akari_wchar *pool,
                      size_t pool_cap)
{
    const akari_wchar *p = src;
    int in_quotes = 0;
    int argc = 0;
    size_t used = 0;

    /* Leading whitespace: first argument is an empty string
     * (CommandLineToArgvW documented behavior). */
    if (*p == 0x20 || *p == 0x09) {
        if (vec) {
            vec[argc] = (pool_cap > 0) ? pool : NULL;
        }
        if (pool && pool_cap > 0) {
            pool[0] = 0;
        }
        used = 1;
        argc = 1;
        while (*p == 0x20 || *p == 0x09) {
            p++;
        }
    }

    for (;;) {
        int token_started = 0;

        while (*p == 0x20 || *p == 0x09) {
            p++;
        }
        if (*p == 0) {
            break;
        }
        if (vec) {
            vec[argc] = (used < pool_cap) ? pool + used : NULL;
        }

        while (*p) {
            akari_wchar c = *p;

            if (c == 0x5C) {                    /* '\' */
                size_t run = 0;
                size_t i;

                while (p[run] == 0x5C) {
                    run++;
                }
                if (p[run] == 0x22) {           /* quote follows run */
                    for (i = 0; i < run / 2u; i++) {
                        put_wc(pool, &used, 0x5C);
                    }
                    if (run & 1u) {
                        put_wc(pool, &used, 0x22); /* literal quote */
                    } else {
                        in_quotes = !in_quotes;    /* toggling quote */
                    }
                    p += run + 1u;
                } else {
                    for (i = 0; i < run; i++) {
                        put_wc(pool, &used, 0x5C);
                    }
                    p += run;
                }
                token_started = 1;
                continue;
            }
            if (c == 0x22) {                    /* '"' */
                if (in_quotes && p[1] == 0x22) {
                    put_wc(pool, &used, 0x22);  /* "" -> literal quote */
                    p += 2u;
                } else {
                    in_quotes = !in_quotes;
                    p++;
                }
                token_started = 1;
                continue;
            }
            if (!in_quotes && (c == 0x20 || c == 0x09)) {
                break;                          /* end of argument */
            }
            put_wc(pool, &used, c);
            p++;
            token_started = 1;
        }

        if (token_started) {
            if (pool && used < pool_cap) {
                pool[used] = 0;
                used++;
            } else if (pool && used >= pool_cap && pool_cap > 0) {
                pool[pool_cap - 1] = 0;
            }
        }
        argc++;
    }

    return argc;
}

/* Pointer to the character just after the argv[0] token in the raw
 * command line -- the position WinMain's lpCmdLine must point at (the
 * command line "excluding the program name", per the CE WinMain
 * documentation).  Applies the same quote/backslash rules as
 * parse_pass so the boundary is identical to the parser's.  A line
 * that starts with whitespace has an empty argv[0] and the tail
 * therefore starts at the first character. */
static const akari_wchar *tail_after_argv0(const akari_wchar *s)
{
    const akari_wchar *p = s;
    int in_quotes = 0;

    if (*p == 0x20 || *p == 0x09) {
        return p;
    }
    while (*p) {
        akari_wchar c = *p;

        if (c == 0x5C) {
            size_t run = 0;

            while (p[run] == 0x5C) {
                run++;
            }
            if (p[run] == 0x22) {
                if (!(run & 1u)) {
                    in_quotes = !in_quotes;
                }
                p += run + 1u;
            } else {
                p += run;
            }
            continue;
        }
        if (c == 0x22) {
            if (in_quotes && p[1] == 0x22) {
                p += 2u;
                continue;
            }
            in_quotes = !in_quotes;
            p++;
            continue;
        }
        if (!in_quotes && (c == 0x20 || c == 0x09)) {
            break;
        }
        p++;
    }
    return p;
}

/* ------------------------------------------------------------------ */
/* Wide -> narrow conversion                                          */
/* ------------------------------------------------------------------ */

static w2m_fn g_w2m = NULL;

static void resolve_w2m(void)
{
    static const akari_wchar module_name[] = {
        'c', 'o', 'r', 'e', 'd', 'l', 'l', '.', 'd', 'l', 'l', 0
    };
    static const akari_wchar w2m_name[] = {
        'W', 'i', 'd', 'e', 'C', 'h', 'a', 'r', 'T', 'o',
        'M', 'u', 'l', 't', 'i', 'B', 'y', 't', 'e', 0
    };
    akari_handle core;
    void *addr;

    if (g_w2m) {
        return;
    }
    core = GetModuleHandleW(module_name);
    if (!core) {
        return;
    }
    addr = GetProcAddressW(core, w2m_name);
    g_w2m = (w2m_fn) (uintptr_t) addr;
}

/* Convert one wide string to CP_ACP; zero-initialised LocalAlloc
 * block, or NULL on failure. */
static char *w2n_one(const akari_wchar *w, size_t len)
{
    char *nb;

    if (g_w2m) {
        int need = g_w2m(CP_ACP, 0, w, (int) len, NULL, 0, NULL, NULL);

        if (need > 0) {
            int conv;

            nb = (char *) LocalAlloc(LPTR, (size_t) need + 1u);
            if (!nb) {
                return NULL;
            }
            conv = g_w2m(CP_ACP, 0, w, (int) len, nb, need, NULL, NULL);
            if (conv > 0) {
                nb[conv] = '\0';
                return nb;
            }
            LocalFree(nb);
        }
    }
    /* Lossy fallback (converter absent from the OS image): 7-bit
     * passthrough, every other code unit becomes '?'. */
    nb = (char *) LocalAlloc(LPTR, len + 1u);
    if (nb) {
        size_t k;

        for (k = 0; k < len; k++) {
            nb[k] = (w[k] < 0x80u) ? (char) w[k] : '?';
        }
        nb[len] = '\0';
    }
    return nb;
}

/* ------------------------------------------------------------------ */
/* Public entry points                                                */
/* ------------------------------------------------------------------ */

akari_handle akari_image_handle(void)
{
    return GetModuleHandleW(NULL);
}

void akari_init_args(void)
{
    static akari_wchar modname[MAX_PATH16];
    akari_wchar *cmd;
    size_t cmd_len;
    int have_cmd;

    resolve_w2m();
    cmd = GetCommandLineW();
    _wcmdln = cmd;      /* may be NULL on a broken OS image */
    if (!cmd) {
        return;
    }
    cmd_len = wcs_len(cmd);
    have_cmd = (cmd[0] != 0);

    modname[0] = 0;
    if (!have_cmd) {
        /* Empty command line: CommandLineToArgvW semantics -- argv[0]
         * is the full path of the current executable. */
        akari_dword got = GetModuleFileNameW(akari_image_handle(),
                                             modname, MAX_PATH16);

        if (got >= MAX_PATH16) {
            got = MAX_PATH16 - 1u;
        }
        modname[got] = 0;
    }

    if (!have_cmd && modname[0] != 0) {
        /* Single-argument case: argv[0] = module path, empty tail. */
        size_t mlen = wcs_len(modname);
        size_t pool_chars = mlen + 2u;
        size_t bytes = 2u * sizeof(akari_wchar *) +
                       pool_chars * sizeof(akari_wchar);
        akari_wchar **vec = (akari_wchar **) LocalAlloc(LPTR, bytes);
        akari_wchar *pool;
        size_t k;

        if (!vec) {
            return;
        }
        pool = (akari_wchar *) ((char *) vec + 2u * sizeof(akari_wchar *));
        __argc = 1;
        __wargv = vec;
        vec[0] = pool;
        vec[1] = NULL;
        for (k = 0; k < mlen; k++) {
            pool[k] = modname[k];
        }
        pool[mlen] = 0;
        _wcmdtail = cmd;    /* points at the NUL: empty tail */
    } else {
        int argc = parse_pass(cmd, NULL, NULL, 0);
        size_t pool_chars;
        size_t bytes;
        akari_wchar **vec;
        akari_wchar *pool;
        int n;

        if (argc < 1) {
            argc = 1;
        }
        pool_chars = cmd_len + (size_t) argc + 2u;
        bytes = (size_t) (argc + 1) * sizeof(akari_wchar *) +
                pool_chars * sizeof(akari_wchar);
        vec = (akari_wchar **) LocalAlloc(LPTR, bytes);
        if (!vec) {
            return;
        }
        pool = (akari_wchar *) ((char *) vec +
                                (size_t) (argc + 1) * sizeof(akari_wchar *));
        n = parse_pass(cmd, vec, pool, pool_chars);
        if (n < 1) {
            n = 1;
            vec[0] = pool;
            pool[0] = 0;
        }
        vec[n] = NULL;
        __argc = n;
        __wargv = vec;
        _wcmdtail = (akari_wchar *) (uintptr_t) tail_after_argv0(cmd);
    }

    /* Narrow argv for main()/argc style entry points. */
    {
        char **nv = (char **) LocalAlloc(LPTR,
                                         (size_t) (__argc + 1) *
                                             sizeof(char *));
        int a;

        __argv = nv;
        if (nv) {
            for (a = 0; a < __argc; a++) {
                if (__wargv && __wargv[a]) {
                    nv[a] = w2n_one(__wargv[a], wcs_len(__wargv[a]));
                } else {
                    nv[a] = NULL;
                }
            }
            nv[__argc] = NULL;
        }
    }

    /* Narrow copy of the whole command line (_acmdln). */
    if (!have_cmd && modname[0] != 0) {
        _acmdln = w2n_one(modname, wcs_len(modname));
    } else {
        _acmdln = w2n_one(cmd, cmd_len);
    }
}

void akari_run_ctors(void)
{
    /* Ranges of MS-style initializer pointers (.CRT$XI* / .CRT$XC*)
     * are walked first-to-last: Microsoft documents alphabetical
     * merging of the .CRT$X* family and lld reproduces it (verified;
     * see the notes at the top of this file).  The .ctors list is
     * walked backward per the layout notes above: entries run from
     * the end of each per-object block, objects in reverse link
     * order. */
    init_fn *p;

    for (p = xi_begin; p < xi_end; p++) {
        if (*p) {
            (*p)();
        }
    }
    for (p = xc_begin; p < xc_end; p++) {
        if (*p) {
            (*p)();
        }
    }
    if (__CTOR_LIST__) {
        init_fn *l = __CTOR_LIST__;
        size_t n = 0;

        if ((uintptr_t) l[0] == (uintptr_t) -1) {
            l++;        /* leading -1 sentinel (lld) */
        }
        while (l[n]) {
            n++;
        }
        while (n > 0) {
            n--;
            if (l[n]) {
                l[n]();
            }
        }
    }
}

void akari_run_dtors(void)
{
    /* .dtors words are stored symmetrically to the .ctors words
     * (target-dependent per-object order; verified) and objects are
     * concatenated in link order, so running the list forward yields
     * the exact LIFO mirror of akari_run_ctors: last-constructed
     * entries are destroyed first (see the layout notes at the top
     * of this file). */
    if (__DTOR_LIST__) {
        init_fn *l = __DTOR_LIST__;
        size_t n = 0;
        size_t i;

        if ((uintptr_t) l[0] == (uintptr_t) -1) {
            l++;
        }
        while (l[n]) {
            n++;
        }
        for (i = 0; i < n; i++) {
            l[i]();
        }
    }
}

/* ------------------------------------------------------------------ */
/* x86-only: ___main                                                  */
/*                                                                     */
/* Clang for i686 windows-gnu targets compiles a call to ___main at
 * the top of every user main() (observed in this toolchain's object
 * output), so the symbol must exist in every such link.  Here the CRT
 * entry point has already run every constructor before the user
 * function is called, so ___main is an empty no-op.  It is defined
 * weak so that a consumer C library that provides its own
 * implementation overrides it.                                        */
/* ------------------------------------------------------------------ */

#if AKARI_CPU_X86

void akari_x86_main_hook(void) AKARI_ENTRY("___main") WEAK;

void akari_x86_main_hook(void)
{
}

#endif /* AKARI_CPU_X86 */
