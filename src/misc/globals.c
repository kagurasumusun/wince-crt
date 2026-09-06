/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * globals.c -- CRT global variables (__argc/__argv, environ, _iob_func).
 */
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>

int        __argc = 0;
char     **__argv = NULL;
char     **__argv_dll = NULL;
wchar_t  **__wargv = NULL;
char     **_environ = NULL;
wchar_t  **_wenviron = NULL;
int        _fmode = 0;
int        _commode = 0;
int        _daylight = 0;
long       _dstbias = 0;
long       _timezone = 0;
char      *_tzname[2] = { "UTC", "UTC" };
int        _doserrno = 0;
char      *_acmdln = NULL;
wchar_t   *_wcmdln = NULL;
unsigned int _akari_sys_blocksize = 4096;
int        _akari_newmode = 0;
int        _mb_cur_max = 1;

/* MSVCRT __iob_func() returns a pointer to the start of the FILE*
 * table (iob[0]=stdin, [1]=stdout, [2]=stderr). */
FILE **__iob_func(void) { return (FILE**)&stdin; }

/* __p___argv / __p___argc / __p___wargv give pointer-to-pointer
 * access required by MSVCRT-ABI code. */
int      **__p___argc(void)  { return &__argc; }
char    ****__p___argv(void)  { return &__argv; }
wchar_t ****__p___wargv(void) { return &__wargv; }
char    ****__p__environ(void) { return &_environ; }
wchar_t ****__p__wenviron(void){ return &_wenviron; }

/* _initterm / _initterm_e call function pointers between two
 * table bounds, used for C++ ctor/dtor dispatch. */
void _initterm(void (**start)(void), void (**end)(void)) {
    if (!start) return;
    for (; start < end; start++) if (*start) (**start)();
}
int _initterm_e(int (**start)(void), int (**end)(void)) {
    if (!start) return 0;
    for (; start < end; start++) if (*start) { int r = (**start)(); if (r) return r; }
    return 0;
}

/* Alternate underscored/imp aliases used by legacy code */
int *_imp____argc = &__argc;
char ***_imp____argv = &__argv;
wchar_t ***_imp____wargv = &__wargv;
char ***_imp___environ = &_environ;
