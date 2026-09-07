/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * compiler.h -- compiler/architecture feature macros for the CRT.
 *
 * Design basis (clean-room; no third-party CRT code consulted):
 *  - Microsoft "Linking to the CRT (Windows CE 5.0)" and "/ENTRY
 *    (Windows CE 5.0)" documentation, which specify the CE entry-point
 *    names and require that entry functions and the functions they
 *    call be defined with the __cdecl calling convention.
 *  - Microsoft C-language documentation on __cdecl/__stdcall name
 *    decoration (x86: leading underscore for C names; __stdcall adds
 *    @n, which Windows CE does not use).
 *  - GNU/Clang "asm labels" documentation for pinning COFF symbol
 *    names, and the MSVC x86 convention that C symbols carry a
 *    leading underscore.
 *
 * The macro set is deliberately small.  Everything that is truly
 * CPU-specific (parameter passing, stack layout, EH tables) belongs
 * to clang's target code generation and to lld, not to this header.
 */
#ifndef _AKARI_COMPILER_H_
#define _AKARI_COMPILER_H_

#if defined(__GNUC__) || defined(__clang__)

#  define NORETURN        __attribute__((noreturn))
#  define WEAK            __attribute__((weak))
#  define USED            __attribute__((used))
#  define SECTION(x)      __attribute__((section(x)))
#  define AKARI_ALIGN(x)  __attribute__((aligned(x)))

/* NOTE on placement: attach NORETURN/WEAK to *declarations* (put the
 * macro after the declarator of a prototype); applying noreturn on a
 * function *definition* triggers a -Wgcc-compat warning in Clang. */

/* AKARI_ENTRY(label): pin a function's COFF symbol name to the exact
 * PE/COFF entry-point spelling, independent of the C-name mangling
 * convention of the target (i386 windows-gnu prefixes C names with an
 * underscore; ARM does not).  Must be written AFTER the declarator:
 *
 *     void WinMainCRTStartup(void) AKARI_ENTRY("WinMainCRTStartup");
 *     void WinMainCRTStartup(void) { ... }
 *
 * (GNU asm labels attach to the declaration they appear in; the
 * attribute-in-the-middle spelling does not compile with Clang.)
 * On host-side build checks the label is dropped. */
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_ENTRY(label) __asm__(label)
#  else
#    define AKARI_ENTRY(label)
#  endif

/* AKARI_DLLIMPORT: __declspec(dllimport).  Functions imported from
 * coredll.dll are referenced through the __imp_<name> pointer slot,
 * which the consumer's coredll.lib import library resolves.  On
 * non-Windows host builds (compile checks only) it expands to
 * nothing. */
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_DLLIMPORT __attribute__((dllimport))
#  else
#    define AKARI_DLLIMPORT
#  endif

/* WINAPI -- calling convention of Windows API entry points.
 *
 * Desktop Win32 (non-CE, 32-bit x86) uses __stdcall; every CE
 * architecture uses the plain C convention:
 *   - ARM / Thumb (CE 4-6): single register-based convention; the
 *     Microsoft CE "/ENTRY (Windows CE 5.0)" topic requires entry
 *     functions to be __cdecl, which on ARM is the only convention.
 *   - x86 CE: Microsoft's CE documentation states the CRT entry
 *     functions (WinMain, wWinMain, DllMain, and the *CRTStartup
 *     functions that call them) must be defined __cdecl, i.e. the
 *     callee does NOT pop arguments and C names are decorated only
 *     with a leading underscore (no @n stdcall decoration).
 *   - MIPS / SuperH: single register-based convention.
 *
 * Hence WINAPI expands to __stdcall ONLY on desktop 32-bit x86 and
 * to nothing everywhere else.  Consumers targeting CE must define
 * _WIN32_WCE (their SDK headers require it anyway); the CRT build
 * passes -D_WIN32_WCE=<ver> itself. */
#  if defined(_WIN32) && !defined(_WIN32_WCE) && \
      (defined(__i386__) || defined(_M_IX86))
#    define WINAPI __attribute__((stdcall))
#  else
#    define WINAPI
#  endif

/* Which CPU family we are on (for x86-only alias spellings of the
 * entry names, mirroring the eVC/older CE x86 tools' leading
 * underscore). */
#  if defined(__i386__) || defined(_M_IX86)
#    define AKARI_CPU_X86 1
#  else
#    define AKARI_CPU_X86 0
#  endif

#else
#  error "Akari requires Clang or GCC."
#endif

#ifndef NULL
#  define NULL ((void *)0)
#endif

#ifdef __cplusplus
#  define AKARI_BEGIN_EXTERN_C extern "C" {
#  define AKARI_END_EXTERN_C   }
#else
#  define AKARI_BEGIN_EXTERN_C
#  define AKARI_END_EXTERN_C
#endif

#endif /* _AKARI_COMPILER_H_ */
