/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stdarg.h -- variable argument list support (compiler builtins).
 */
#ifndef _AKARI_STDARG_H_
#define _AKARI_STDARG_H_

typedef __builtin_va_list va_list;
#define va_start(ap, last)  __builtin_va_start((ap), (last))
#define va_end(ap)          __builtin_va_end((ap))
#define va_arg(ap, type)    __builtin_va_arg((ap), type)
#define va_copy(dst, src)   __builtin_va_copy((dst), (src))

#endif /* _AKARI_STDARG_H_ */
