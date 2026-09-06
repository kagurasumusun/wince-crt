/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * globals.c -- MSVCRT-ABI global variables and accessor functions.
 * These are the symbols that compiled code references for the
 * standard C runtime data.
 */
#include <stddef.h>
#include <akari/compiler.h>

/* FILE is declared by libc headers; we only need forward-declarations
 * for __iob_func, which returns a pointer to an array of FILE*.  We
 * leave the actual stdin/stdout/stderr array to the linked libc, but
 * we must provide __iob_func to satisfy MSVCRT-ABI references. */
struct __akari_iob_s;
extern struct __akari_iob_s **__iob(void);

int        __argc = 0;
char     **__argv = NULL;
unsigned short **__wargv = NULL;
char     **__argv_dll = NULL;
char      *_acmdln = NULL;
unsigned short *_wcmdln = NULL;
char     **_environ = NULL;
unsigned short **_wenviron = NULL;
int        _fmode = 0;
int        _commode = 0;
int        _doserrno = 0;
int        _mb_cur_max = 1;
int        _akari_newmode = 0;

/* MSVCRT __iob_func() returns &__iob[0]. */
struct __akari_iob_s ***__iob_func(void);
struct __akari_iob_s ***__iob_func(void) { return (struct __akari_iob_s ***)__iob; }

/* __p___argv / friends: pointer-to-pointer accessors */
int        **__p___argc(void)  { return &__argc; }
char      ****__p___argv(void)  { return &__argv; }
unsigned short ****__p___wargv(void) { return &__wargv; }
char      ****__p__environ(void) { return &_environ; }

/* _initterm / _initterm_e: call function-pointer tables (C++ ctors) */
void _initterm(void (**start)(void), void (**end)(void)) {
    if (!start) return;
    for (; start < end; start++) if (*start) (**start)();
}
int _initterm_e(int (**start)(void), int (**end)(void)) {
    if (!start) return 0;
    for (; start < end; start++) if (*start) { int r = (**start)(); if (r) return r; }
    return 0;
}

/* __imp__ prefixed symbols are used by dllimport references */
int *_imp____argc = &__argc;
char ***_imp____argv = &__argv;
