/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt.h -- Declarations of the MSVCRT-ABI global symbols that Akari
 * defines.
 *
 * Akari does NOT provide the C library.  exit(), malloc(), printf(),
 * errno, atexit(), stdio FILE objects, string/math/stdlib/ctype,
 * setjmp/longjmp, __stack_chk_guard/__stack_chk_fail all come from
 * whichever C library the consumer links against (coredll.dll,
 * newlib, llvm-libc).
 *
 * What Akari DOES define, and what this header exposes, are the
 * per-process MSVCRT data globals that no DLL exports (every Win32
 * CRT defines its own copies).
 */
#ifndef _AKARI_CRT_H_
#define _AKARI_CRT_H_

#include <stddef.h>
#include <akari/compiler.h>

AKARI_BEGIN_EXTERN_C

extern int              __argc;       /* number of parsed arguments */
extern char           **__argv;       /* narrow argument vector (CP_ACP or lossy ASCII) */
extern wchar_t        **__wargv;      /* wide (native UTF-16) argument vector */
extern char            *_acmdln;      /* narrow command tail (== __argv[0] when set) */
extern wchar_t         *_wcmdln;      /* full wide command line (GetCommandLineW() result) */
extern wchar_t         *_wcmdtail;    /* tail pointer handed to WinMain lpCmdLine */
extern int              _fmode;       /* default file translation mode (0 = _O_BINARY) */
extern int              _doserrno;    /* errno <-> GetLastError mapping (maintained by libc) */
extern int              _commode;     /* default commit-on-write flag */

AKARI_END_EXTERN_C
#endif /* _AKARI_CRT_H_ */
