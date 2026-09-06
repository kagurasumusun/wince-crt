/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * atexit.c -- minimal atexit / __cxa_atexit tables.
 * Uses the standard malloc/free symbols which are provided by whatever
 * C library is linked alongside this CRT (typically coredll msvcrt
 * exports or a libc port).
 */
#include <stddef.h>
#include <akari/compiler.h>

extern void *malloc(unsigned long);
extern void  free(void *);

typedef void (*atexit_fn)(void);
typedef void (*cxa_atexit_fn)(void *);

#define AKARI_ATEXIT_INITIAL 32
static atexit_fn *_akari_atexit_tbl = NULL;
static int _akari_atexit_cnt = 0;
static int _akari_atexit_cap = 0;

void _akari_atexit_init(void)
{
    if (_akari_atexit_tbl) return;
    _akari_atexit_tbl = (atexit_fn *)malloc(sizeof(atexit_fn) * AKARI_ATEXIT_INITIAL);
    if (_akari_atexit_tbl) {
        _akari_atexit_cap = AKARI_ATEXIT_INITIAL;
        _akari_atexit_cnt = 0;
    }
}

int atexit(void (*fn)(void))
{
    if (!fn) return -1;
    _akari_atexit_init();
    if (!_akari_atexit_tbl) return -1;
    if (_akari_atexit_cnt >= _akari_atexit_cap) {
        /* grow */
        int ncap = _akari_atexit_cap * 2;
        atexit_fn *nt = (atexit_fn *)malloc(sizeof(atexit_fn) * (unsigned long)ncap);
        if (!nt) return -1;
        for (int i = 0; i < _akari_atexit_cnt; i++) nt[i] = _akari_atexit_tbl[i];
        free(_akari_atexit_tbl);
        _akari_atexit_tbl = nt;
        _akari_atexit_cap = ncap;
    }
    _akari_atexit_tbl[_akari_atexit_cnt++] = fn;
    return 0;
}

int _onexit(void (*fn)(void)) { return atexit(fn); }

/* __cxa_atexit: dso handle is ignored (no shared-object unload on CE) */
int __cxa_atexit(void (*dtor)(void *), void *obj, void *dso)
{
    (void)obj; (void)dso;
    /* CE has no dynamic-object unload notifications; we simply register
     * the dtor as a plain atexit function and ignore the obj argument.
     * This is sufficient for the common case (global destructors). */
    return atexit((atexit_fn)dtor);
}

void _akari_atexit_fini(void)
{
    if (!_akari_atexit_tbl) return;
    for (int i = _akari_atexit_cnt - 1; i >= 0; i--) {
        if (_akari_atexit_tbl[i]) _akari_atexit_tbl[i]();
    }
    free(_akari_atexit_tbl);
    _akari_atexit_tbl = NULL;
    _akari_atexit_cnt = _akari_atexit_cap = 0;
}
