/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * compiler.h -- compiler macros used across the CRT sources.
 */
#ifndef _AKARI_COMPILER_H_
#define _AKARI_COMPILER_H_

#if defined(__GNUC__) || defined(__clang__)
#  define NORETURN    __attribute__((noreturn))
#  define WEAK        __attribute__((weak))
#  define USED        __attribute__((used))
#  if defined(_WIN32) || defined(_WIN64) || defined(__i386__)
#    define WINAPI    __attribute__((stdcall))
#  else
#    define WINAPI    /* ignored on host / non-x86 */
#  endif
#  if defined(_WIN32) || defined(_WIN64)
#    define AKARI_DLLIMPORT __attribute__((dllimport))
#    define AKARI_DLLEXPORT __attribute__((dllexport))
#  else
#    define AKARI_DLLIMPORT
#    define AKARI_DLLEXPORT
#  endif
#else
#  define NORETURN
#  define WEAK
#  define USED
#  define WINAPI      __stdcall
#  define AKARI_DLLIMPORT __declspec(dllimport)
#  define AKARI_DLLEXPORT __declspec(dllexport)
#endif

#ifndef __declspec
#  if defined(_DEBUG_HOSTCHECK_) || !defined(_WIN32)
#    define __declspec(p) /* empty */
#  endif
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

#endif /* _AKARI_COMPILER_H_ */
