/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt.h -- Declarations of the MSVCRT-ABI global symbols that Akari
 * defines.
 *
 * Akari does NOT provide the C library.  exit(), malloc(), printf(),
 * errno, atexit(), the stdio FILE objects, string/math/stdlib/ctype
 * routines, etc., all come from whichever C library the consumer
 * links against -- typically coredll.dll on standard Windows CE
 * images, or a static libc such as llvm-libc / newlib.
 *
 * What Akari DOES define, and what this header exposes, are the
 * per-process MSVCRT data globals that no DLL exports (they are
 * owned by the CRT image in every Win32 toolchain): __argc, __argv,
 * __wargv, _acmdln, _wcmdln, _fmode, _doserrno, _commode.
 */
#ifndef _AKARI_CRT_H_
#define _AKARI_CRT_H_

#include <stddef.h>
#include <akari/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int              __argc;
extern char           **__argv;
extern wchar_t        **__wargv;
extern char            *_acmdln;     /* narrow command tail (argv[0]) */
extern wchar_t         *_wcmdln;     /* wide command line as returned by GetCommandLineW() */
extern int              _fmode;      /* default file translation mode */
extern int              _doserrno;   /* O.S. error mapping */
extern int              _commode;    /* default commit-on-write flag */

#ifdef __cplusplus
}
#endif
#endif /* _AKARI_CRT_H_ */
