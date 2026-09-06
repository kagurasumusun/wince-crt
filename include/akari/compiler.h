/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * akari/compiler.h -- compiler helpers (no non-standard attributes)
 */
#ifndef _AKARI_COMPILER_H_
#define _AKARI_COMPILER_H_

#ifndef INLINE
#  if defined(__cplusplus)
#    define INLINE inline
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define INLINE inline
#  else
#    define INLINE static
#  endif
#endif

#ifndef NORETURN
#  if defined(__GNUC__) || defined(__clang__)
#    define NORETURN __attribute__((noreturn))
#  else
#    define NORETURN
#  endif
#endif

#ifndef UNUSED
#  define UNUSED(x) ((void)(x))
#endif

#endif /* _AKARI_COMPILER_H_ */
