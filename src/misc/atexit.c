/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * atexit.c -- registration and calling of exit handlers.
 *
 * We allocate a dynamic atexit table that grows on demand. Registered
 * handlers are called in reverse order (LIFO) at exit().
 */
#include <stddef.h>
#include <stdlib.h>
#include <akari/winnt.h>
#include <akari/compiler.h>

#define AKARI_ATEXIT_INITIAL  32
#define AKARI_ATEXIT_MAX      1024

typedef void (*atexit_fn)(void);

static atexit_fn *_akari_atexit_tbl = NULL;
static size_t     _akari_atexit_cnt = 0;
static size_t     _akari_atexit_cap = 0;

void _akari_atexit_fini(void);

int atexit(void (*fn)(void))
{
    if (_akari_atexit_tbl == NULL) {
        _akari_atexit_tbl = (atexit_fn *)malloc(sizeof(atexit_fn) * AKARI_ATEXIT_INITIAL);
        if (!_akari_atexit_tbl) return -1;
        _akari_atexit_cap = AKARI_ATEXIT_INITIAL;
        _akari_atexit_cnt = 0;
    }
    if (_akari_atexit_cnt >= _akari_atexit_cap) {
        size_t ncap = _akari_atexit_cap * 2;
        if (ncap > AKARI_ATEXIT_MAX) return -1;
        atexit_fn *nt = (atexit_fn *)realloc(_akari_atexit_tbl,
                                             sizeof(atexit_fn) * ncap);
        if (!nt) return -1;
        _akari_atexit_tbl = nt;
        _akari_atexit_cap = ncap;
    }
    _akari_atexit_tbl[_akari_atexit_cnt++] = fn;
    return 0;
}

static int _akari_atexit_registered = 0;

void _akari_atexit_register(void)
{
    if (!_akari_atexit_registered) { _akari_atexit_registered = 1; }
}

void _akari_atexit_fini(void)
{
    if (_akari_atexit_tbl) {
        size_t i = _akari_atexit_cnt;
        while (i--) {
            if (_akari_atexit_tbl[i]) _akari_atexit_tbl[i]();
        }
        free(_akari_atexit_tbl);
        _akari_atexit_tbl = NULL;
        _akari_atexit_cnt = _akari_atexit_cap = 0;
    }
}

/* Allow start-up code to call destructors */
void _akari_run_global_dtors(void) { _akari_atexit_fini(); }
