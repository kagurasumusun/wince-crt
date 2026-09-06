/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * compiler.h -- Compiler/architecture macros used across the CRT.
 *
 * Clean-room; written against Clang/GCC documentation and the
 * Windows CE ABI.
 */
#ifndef _AKARI_COMPILER_H_
#define _AKARI_COMPILER_H_

#if defined(__GNUC__) || defined(__clang__)
#  define NORETURN       __attribute__((noreturn))
#  define WEAK           __attribute__((weak))
#  define USED           __attribute__((used))
#  define SECTION(x)     __attribute__((section(x)))
#  define NOINLINE       __attribute__((noinline))
#  define AKARI_ALIGN(x) __attribute__((aligned(x)))

/* DLL import/export markers.  On COFF these produce the
 * __imp_<name> stubs that lld's auto-import resolves.  On hostcheck
 * they expand to nothing. */
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_DLLIMPORT __attribute__((dllimport))
#    define AKARI_DLLEXPORT __attribute__((dllexport))
#  else
#    define AKARI_DLLIMPORT
#    define AKARI_DLLEXPORT
#  endif

/* WINAPI -- the calling convention for Win32 API callbacks.
 *
 * On DESKTOP Win32 (non-CE, 32-bit x86) this is __stdcall (callee-
 * pops, @N decoration).  On WINDOWS CE this is the platform default
 * C convention on every architecture:
 *   - ARM (AAPCS): only one convention.
 *   - x86 CE: coredll exports are __cdecl (NO @N decorations).
 *   - MIPS / SH: only one convention.
 *
 * We therefore expand WINAPI to __stdcall ONLY for non-CE 32-bit
 * x86 desktop targets.  On every supported CE architecture it is
 * empty.
 */
#  if defined(_WIN32) && !defined(_WIN32_WCE) && (defined(__i386__) || defined(_M_IX86))
#    define WINAPI __attribute__((stdcall))
#  else
#    define WINAPI /* nothing */
#  endif

/* Pin entry-point symbol names to the undecorated names the PE/COFF
 * loader expects, regardless of the target C-mangling convention.
 * On i386 windows-gnu, clang prepends an underscore to extern "C"
 * symbols; using __asm__(name) forces the correct plain name so the
 * loader can resolve -Wl,-entry:WinMainCRTStartup without the
 * consumer having to add a leading underscore on x86.
 *
 * We only apply the asm label on actual Windows builds; on hostcheck
 * we leave names as-is (no need for the PE convention). */
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_ENTRY(name) __asm__(name)
#  else
#    define AKARI_ENTRY(name)
#  endif
#else
#  error "Akari requires Clang or GCC.  The supported toolchain is LLVM/Clang/lld."
#endif

/* Default _WIN32_WCE baseline.  Consumers can override with
 * -D_WIN32_WCE=0x501 etc. on the command line. */
#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(_WIN32_WINNT) && !defined(_DEBUG_HOSTCHECK_)
#  define _WIN32_WCE 0x0400
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

/* Extern "C" guards for headers. */
#ifdef __cplusplus
#  define AKARI_BEGIN_EXTERN_C extern "C" {
#  define AKARI_END_EXTERN_C   }
#else
#  define AKARI_BEGIN_EXTERN_C
#  define AKARI_END_EXTERN_C
#endif

#endif /* _AKARI_COMPILER_H_ */
