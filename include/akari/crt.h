/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt.h -- Declarations of the MSVCRT-ABI global symbols that Akari
 * defines (__argc/__argv/__wargv/_acmdln/_fmode).
 *
 * The C library (coredll / newlib / llvm-libc / msvcrt) provides
 * exit(), malloc(), printf(), errno, atexit, etc.; this header is
 * intentionally tiny.
 */
#ifndef _AKARI_CRT_H_
#define _AKARI_CRT_H_

#include <akari/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int              __argc;
extern char           **__argv;
extern unsigned short **__wargv;
extern char            *_acmdln;
extern int              _fmode;
extern int              _doserrno;

#ifdef __cplusplus
}
#endif
#endif /* _AKARI_CRT_H_ */
