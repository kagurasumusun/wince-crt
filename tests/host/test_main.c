/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * test_main.c -- host-side self test for the command-line machinery
 * in runtime.c.  Compile with the HOST compiler together with
 * runtime.c; the coredll.dll imports are provided by the stubs below.
 *
 * The wide strings here model UTF-16 code units as uint16_t, exactly
 * like runtime.c, so the test runs unchanged on 32-bit-wchar hosts.
 *
 * The vectors encode the parsing rules published in Microsoft's
 * documentation (CommandLineToArgvW / "Parsing C command-line
 * arguments"): whitespace delimiters, quoting, 2n/2n+1 backslash
 * handling, ""-inside-quotes, unterminated quotes, leading
 * whitespace, and the empty-command-line -> executable-path rule.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <akari/compiler.h>
#include <akari/internal.h>

/* ------------------------------------------------------------------ */
/* coredll stubs                                                      */
/* ------------------------------------------------------------------ */

static akari_wchar *g_cmdline;         /* served by GetCommandLineW   */
static const akari_wchar *g_modpath;   /* served by GetModuleFileNameW */
static int g_no_w2m;                   /* simulate converter-less OS   */

static int w2m_stub(akari_dword cp, akari_dword flags,
                    const akari_wchar *w, int wlen,
                    char *out, int outcap,
                    const char *defch, const int *used)
{
    int i;
    (void) cp;
    (void) flags;
    (void) defch;
    (void) used;
    if (wlen < 0) {
        wlen = 0;
        while (w[wlen]) {
            wlen++;
        }
    }
    for (i = 0; i < wlen; i++) {
        if (out && i < outcap) {
            out[i] = (w[i] < 0x80u) ? (char) w[i] : '?';
        }
    }
    return wlen;
}

akari_handle GetModuleHandleW(const akari_wchar *name)
{
    static int core_marker;
    (void) name;
    if (name == NULL) {
        return &core_marker; /* instance handle */
    }
    if (name[0] == 'c' && name[1] == 'o') {
        return &core_marker; /* "coredll.dll" */
    }
    return NULL;
}

akari_wchar *GetCommandLineW(void)
{
    return g_cmdline ? g_cmdline : (akari_wchar *) 0;
}

akari_dword GetModuleFileNameW(akari_handle h, akari_wchar *buf,
                               akari_dword cap)
{
    size_t n;
    size_t i;
    (void) h;
    if (!g_modpath) {
        return 0;
    }
    n = 0;
    while (g_modpath[n]) {
        n++;
    }
    if (n >= cap) {
        n = (size_t) cap - 1u;
    }
    for (i = 0; i < n; i++) {
        buf[i] = g_modpath[i];
    }
    buf[n] = 0;
    return (akari_dword) n;
}

void *LocalAlloc(akari_dword flags, size_t bytes)
{
    void *p;
    (void) flags;
    p = calloc(1, bytes);
    return p;
}

void LocalFree(void *p)
{
    free(p);
}

void *GetProcAddressW(akari_handle h, const akari_wchar *name)
{
    static const akari_wchar w2m_name[] = {
        'W', 'i', 'd', 'e', 'C', 'h', 'a', 'r', 'T', 'o',
        'M', 'u', 'l', 't', 'i', 'B', 'y', 't', 'e', 0
    };
    size_t i;
    (void) h;
    for (i = 0; name[i] && w2m_name[i]; i++) {
        if (name[i] != w2m_name[i]) {
            break;
        }
    }
    if (!g_no_w2m && name[i] == 0 && w2m_name[i] == 0) {
        return (void *) (uintptr_t) w2m_stub;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* test harness                                                       */
/* ------------------------------------------------------------------ */

static int failures = 0;
static int checks = 0;

static void check(int cond, const char *what)
{
    checks++;
    if (!cond) {
        failures++;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

static size_t wlen16(const akari_wchar *s)
{
    size_t n = 0;
    while (s && s[n]) {
        n++;
    }
    return n;
}

static int wseq(const akari_wchar *a, const akari_wchar *b)
{
    size_t i;
    if (!a || !b) {
        return a == b;
    }
    for (i = 0; a[i] && b[i]; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return a[i] == b[i];
}

static void set_cmdline(const char *ascii)
{
    size_t n = strlen(ascii);
    size_t i;
    free(g_cmdline);
    g_cmdline = (akari_wchar *) calloc(n + 1, sizeof(akari_wchar));
    for (i = 0; i < n; i++) {
        g_cmdline[i] = (akari_wchar) (unsigned char) ascii[i];
    }
}

/* argv[i] expected == ascii text; NULL terminates list.  When
 * argv[0] is expected to equal the module path, pass path_magic. */
static const char *PATH_MAGIC = "<modpath>";

static void run_vector(const char *raw, const char *tail_prefix,
                       const char *const *expect)
{
    char msg[256];
    int i;
    int tail_len = (int) strlen(tail_prefix);

    set_cmdline(raw);
    g_modpath = (const akari_wchar *) 0;
    _acmdln = NULL;
    __argc = 0;
    __argv = NULL;
    __wargv = NULL;
    akari_init_args();

    snprintf(msg, sizeof(msg), "argc for [%s]", raw);
    for (i = 0; expect[i] != NULL; i++) {
    }
    check(__argc == i, msg);

    for (i = 0; expect[i] != NULL; i++) {
        snprintf(msg, sizeof(msg), "argv[%d] for [%s]", i, raw);
        if (strcmp(expect[i], PATH_MAGIC) == 0) {
            check(wseq(__wargv[i], g_modpath), msg);
        } else {
            size_t n = strlen(expect[i]);
            size_t k;
            int ok = __wargv && wlen16(__wargv[i]) == n;
            for (k = 0; ok && k < n; k++) {
                if (__wargv[i][k] != (akari_wchar) (unsigned char)
                                         expect[i][k]) {
                    ok = 0;
                }
            }
            check(ok, msg);
        }
        if (expect[i][0] != '<') { /* narrow check where printable */
            size_t n = strlen(expect[i]);
            check(__argv && __argv[i] && strncmp(__argv[i], expect[i], n) == 0 &&
                  __argv[i][n] == '\0', msg);
        }
    }

    /* tail: starts with tail_prefix and points inside the raw buffer */
    {
        ptrdiff_t off = (ptrdiff_t) (_wcmdtail - g_cmdline);
        int ok = off >= 0 && (size_t) off <= strlen(raw);
        size_t k;
        for (k = 0; ok && k < (size_t) tail_len; k++) {
            if (_wcmdtail[k] != (akari_wchar) (unsigned char) tail_prefix[k]) {
                ok = 0;
            }
        }
        snprintf(msg, sizeof(msg), "wcmdtail prefix for [%s]", raw);
        check(ok, msg);
    }
}

static void run_module_path_vector(void)
{
    static const akari_wchar path[] = {
        'C', ':', '\\', 'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
        'F', 'i', 'l', 'e', 's', '\\', 'a', 'p', 'p', '.', 'e', 'x', 'e', 0
    };

    g_modpath = path;
    set_cmdline("");
    __argc = 0;
    akari_init_args();
    check(__argc == 1, "empty line argc");
    check(wseq(__wargv[0], path), "empty line argv0 == module path");
    check(wseq(_wcmdtail, (const akari_wchar *) 0) ||
          _wcmdtail[0] == 0, "empty line tail empty");
    check(__argv && __argv[0] &&
          strcmp(__argv[0], "C:\\Program Files\\app.exe") == 0,
          "narrow argv0 == module path");
    g_modpath = (const akari_wchar *) 0;
}

static void run_empty_no_path_vector(void)
{
    /* Empty command line AND GetModuleFileNameW failure: the module
     * path fallback has nothing to use; the documented behavior is a
     * single empty argv[0] (argc == 1). */
    static const akari_wchar empty0[] = { 0 };

    g_modpath = (const akari_wchar *) 0;
    set_cmdline("");
    __argc = 0;
    __argv = NULL;
    __wargv = NULL;
    akari_init_args();
    check(__argc == 1, "empty line (no path) argc == 1");
    check(__wargv && __wargv[0] != NULL && wseq(__wargv[0], empty0),
          "empty line (no path) argv0 == empty string");
    check(__argv && __argv[0] && __argv[0][0] == '\0',
          "empty line (no path) narrow argv0 == empty string");
}

static void run_null_cmdline_vector(void)
{
    /* GetCommandLineW returning NULL (broken OS image): init returns
     * without touching the argument objects; argc stays 0. */
    g_cmdline = (akari_wchar *) 0;
    __argc = 0;
    __argv = NULL;
    __wargv = NULL;
    akari_init_args();
    check(__argc == 0, "NULL command line: argc stays 0");
}

static void run_lossy_vector(void)
{
    /* 0xE9 is not 7-bit: the documented CRT fallback maps it to '?';
     * the WideCharToMultiByte stub below maps it the same way, so
     * both modes share one expectation. */
    static akari_wchar raw[] = {
        'p', 'r', 'o', 'g', ' ', 0xE9, 'l', 'e', 0
    };

    g_cmdline = raw;
    g_no_w2m = 1;            /* converter absent: fallback path */
    __argc = 0;
    akari_init_args();
    check(__argc == 2, "lossy argc");
    check(__argv[1] && __argv[1][0] == '?' &&
          __argv[1][1] == 'l' && __argv[1][2] == 'e' &&
          __argv[1][3] == '\0', "lossy conversion (fallback)");

    g_no_w2m = 0;            /* converter present: import path */
    __argc = 0;
    akari_init_args();
    check(__argc == 2, "lossy argc (import path)");
    check(__argv[1] && __argv[1][0] == '?' &&
          __argv[1][1] == 'l' && __argv[1][2] == 'e' &&
          __argv[1][3] == '\0', "lossy conversion (import path)");

    g_cmdline = (akari_wchar *) 0;
}

int main(void)
{
    const char *const v1[] = { "app.exe", "a", "b", "c", NULL };
    const char *const v2[] = { "a b c", "d", "e", NULL };
    const char *const v3[] = { "ab\"c", "\\", "d", NULL };
    const char *const v4[] = { "a\\\\\\b", "de fg", "h", NULL };
    const char *const v5[] = { "a\\\"b", "c", "d", NULL };
    const char *const v6[] = { "a\\\\b c", "d", "e", NULL };
    const char *const v7[] = { "ab\" c d", NULL };
    const char *const v8[] = { "", "foo", "bar", NULL };
    const char *const v9[] = { "", NULL };
    const char *const v10[] = { "C:\\dir\\file.exe", "-x", NULL };
    const char *const v11[] = { "app", "with   spaces  inside", NULL };
    const char *const v12[] = { "ab", NULL };
    const char *const v13[] = { "app", "a", NULL }; /* trailing ws ignored */
    const char *const v14[] = { "app", "", "x", NULL }; /* empty arg between */
    const char *const v15[] = { "ab", "x", NULL };  /* quotes toggle mid-token */

    /* Fallback path first (the resolver has not run yet), so both
     * converter modes are covered; every later init retries the
     * resolver because a failed resolution is not cached. */
    run_lossy_vector();
    run_module_path_vector();
    run_empty_no_path_vector();
    run_null_cmdline_vector();

    run_vector("app.exe a b c", " a b c", v1);
    run_vector("\"a b c\" d e", " d e", v2);
    run_vector("\"ab\\\"c\" \"\\\\\" d", " \"\\\\\" d", v3);
    run_vector("a\\\\\\b d\"e f\"g h", " d\"e f\"g h", v4);
    run_vector("a\\\\\\\"b c d", " c d", v5);
    run_vector("a\\\\\\\\\"b c\" d e", " d e", v6);
    run_vector("a\"b\"\" c d", "", v7);      /* unterminated tail */
    run_vector("   foo bar", "   foo bar", v8); /* leading ws: empty argv0 */
    run_vector("\"\"", "", v9);              /* empty quoted arg */
    run_vector("\"C:\\dir\\file.exe\" -x", " -x", v10);
    run_vector("app \"with   spaces  inside\"", " \"with   spaces  inside\"",
               v11);
    run_vector("a\"b", "", v12);             /* unterminated quote */
    run_vector("app a   ", " a", v13);       /* trailing ws ignored */
    run_vector("app \"\" x", " \"\" x", v14);/* empty quoted arg between */
    run_vector("a\"\"b x", " x", v15);       /* quotes toggle mid-token */

    if (failures) {
        fprintf(stderr, "hosttest: %d/%d checks FAILED\n", failures,
                checks);
        return 1;
    }
    printf("hosttest: all %d checks passed\n", checks);
    return 0;
}
