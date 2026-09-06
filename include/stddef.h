/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stddef.h -- standard definitions.
 */
#ifndef _AKARI_STDDEF_H_
#define _AKARI_STDDEF_H_

#include <akari/windef.h>

typedef __SIZE_TYPE__   size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __cplusplus
/* On Windows CE wchar_t is 16-bit unsigned (UTF-16). */
#  if defined(_AKARI_WANT_SHORT_WCHAR)
typedef unsigned short  wchar_t;
#  elif defined(_WIN32)
typedef unsigned short  wchar_t;
#  else
typedef __WCHAR_TYPE__  wchar_t;
#  endif
#endif

#ifndef NULL
#   define NULL ((void*)0)
#endif

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif /* _AKARI_STDDEF_H_ */
