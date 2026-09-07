/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt.h -- public data-global declarations provided by the CRT.
 *
 * Scope: Akari is the PE/COFF startup and process-glue layer for
 * programs built with Clang/lld for Windows CE.  It is NOT a C
 * library: malloc/printf/exit/atexit/errno/stdio/string/setjmp all
 * come from whichever C library the consumer links (e.g. the
 * platform's corelibc equivalent built from newlib-style sources, an
 * llvm-libc port, or a C library linked from coredll imports).
 *
 * The symbols declared here are the per-process data objects of the
 * MSVCRT data model (__argc/__argv/...); each EXE and each DLL module
 * that uses them links its own copy from Akari, since the OS DLLs do
 * not provide them.  The wide forms are the native ones on Windows CE
 * (only the Unicode forms of the command-line APIs exist on CE); the
 * narrow forms are synthesized with WideCharToMultiByte(CP_ACP) when
 * the converter is present in the OS image and lossily otherwise.
 *
 * Windows CE has no POSIX environment block: envp is NULL for
 * main()/wmain() and there is no environ.
 */
#ifndef _AKARI_CRT_H_
#define _AKARI_CRT_H_

#include <stddef.h>
#include <akari/compiler.h>

AKARI_BEGIN_EXTERN_C

extern int              __argc;    /* argument count (>= 1; argv[0] set) */
extern char           **__argv;    /* narrow argv (CP_ACP conversion) */
extern wchar_t        **__wargv;   /* native wide argv */
extern char            *_acmdln;   /* narrow copy of the raw command line */
extern wchar_t         *_wcmdln;   /* raw command line (GetCommandLineW) */
extern wchar_t         *_wcmdtail; /* WinMain lpCmdLine: raw tail after argv[0] */
extern int              _fmode;    /* default file translation mode (0) */
extern int              _doserrno; /* errno mapping slot (libc-maintained) */
extern int              _commode;  /* commit mode flag (0) */
extern void            *__dso_handle; /* NULL in EXE; DLL HMODULE in DLLs */

AKARI_END_EXTERN_C
#endif /* _AKARI_CRT_H_ */
