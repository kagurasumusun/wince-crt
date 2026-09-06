/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt.h -- Declarations for MSVCRT-ABI symbols that this CRT exports.
 * This header is intentionally tiny; the CRT does NOT ship libc or
 * Win32 SDK headers -- consumers provide those themselves.
 */
#ifndef _AKARI_CRT_H_
#define _AKARI_CRT_H_

#include <akari/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MSVCRT data globals populated by the startup code */
extern int          __argc;
extern char       **__argv;
extern unsigned short **__wargv;
extern char        *_acmdln;
extern unsigned short *_wcmdln;
extern int          _fmode;
extern int          _doserrno;

/* MSVCRT accessors */
int        **__p___argc(void);
char      ****__p___argv(void);
unsigned short ****__p___wargv(void);
struct __akari_iob_s ***__iob_func(void);
void         _initterm(void (**)(void), void (**)(void));
int          _initterm_e(int (**)(void), int (**)(void));

/* errno accessor (matches MSVCRT _errno() macro) */
int *_errno(void);

#ifdef __cplusplus
}
#endif
#endif /* _AKARI_CRT_H_ */
