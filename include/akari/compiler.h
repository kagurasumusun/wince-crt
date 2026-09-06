/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * compiler.h -- Compiler/architecture macros used across the CRT sources.
 *
 * This header deliberately knows NOTHING about mingw-w64, cegcc, or any
 * other existing CRT.  It is written from the documented Win32 /
 * Windows CE ABI as targetted by LLVM/Clang/lld.
 *
 * Architecture notes for Windows CE (CE 4–6):
 *   - Supported CPU families in CE 4–6 are ARM (v4/v4i/v5/v6/v7 IWMMXT),
 *     x86 (i486+), MIPS (MIPSII/MIPSII_FP/MIPSIV/MIPSIV_FP/MIPS16),
 *     SH3/SH4.  CE 7 also adds ARMv7Thumb2 and later but is out of scope.
 *   - On EVERY CE architecture the Win32 API uses the platform's default
 *     C calling convention.  Desktop Win32's __stdcall convention exists
 *     on x86 Windows only; coredll.dll on x86 CE exports symbols with
 *     __cdecl convention (no @N decorations on names).  On ARM/MIPS/SH
 *     there is only one calling convention anyway.  Therefore WINAPI
 *     expands to nothing for Windows CE targets.
 *   - We do not expose any __stdcall / __fastcall / __thiscall names;
 *     those are desktop-Windows-specific decorations.
 */
#ifndef _AKARI_COMPILER_H_
#define _AKARI_COMPILER_H_

/* Clang or GCC. */
#if defined(__GNUC__) || defined(__clang__)
#  define NORETURN    __attribute__((noreturn))
#  define WEAK        __attribute__((weak))
#  define USED        __attribute__((used))
#  define SECTION(x)  __attribute__((section(x)))
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_DLLIMPORT __attribute__((dllimport))
#    define AKARI_DLLEXPORT __attribute__((dllexport))
#  else
#    define AKARI_DLLIMPORT
#    define AKARI_DLLEXPORT
#  endif
#  if defined(_WIN32) && !defined(_WIN32_WCE) && (defined(__i386__) || defined(_M_IX86))
     /* Desktop Win32 only -- NOT Windows CE. */
#    define WINAPI    __attribute__((stdcall))
#  else
     /* Windows CE on any architecture (ARM / x86 / MIPS / SH) uses the
      * platform default C calling convention (cdecl).  This also covers
      * the hostcheck build and non-Windows targets. */
#    define WINAPI    /* nothing */
#  endif
#else
#  error "Akari requires Clang or GCC.  The target toolchain is LLVM/Clang/lld."
#endif

/* Windows CE SDK compatibility: pull in the version constant.
 * Consumers pass -D_WIN32_WCE=0x400..0x600 on the command line; if none
 * is supplied we assume the conservative baseline CE 4.0. */
#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(_WIN32_WINNT) && !defined(_DEBUG_HOSTCHECK_)
#  define _WIN32_WCE 0x0400
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

#endif /* _AKARI_COMPILER_H_ */
