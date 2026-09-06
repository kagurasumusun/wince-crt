/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_PRINTF_CORE_H_
#define _AKARI_PRINTF_CORE_H_

#include <stddef.h>
#include <stdarg.h>

typedef int (*akari_putc_fn)(void *ctx, int c);
typedef int (*akari_puts_fn)(void *ctx, const char *s, size_t n);

int _akari_format_core(void *ctx, akari_putc_fn putc, akari_puts_fn puts,
                       const char *fmt, va_list ap);

#endif
