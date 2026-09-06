/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * snprintf.c -- sprintf/snprintf/vsnprintf entry points.
 */
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "printf_core.h"

typedef struct {
    char *buf;
    size_t pos;
    size_t cap;
} _sink_t;

static int _sink_putc(void *ctx, int c)
{
    _sink_t *s = (_sink_t *)ctx;
    if (s->buf == NULL) { s->pos++; return 1; }
    if (s->pos + 1 < s->cap) s->buf[s->pos] = (char)c;
    s->pos++;
    return 1;
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    _sink_t sink;
    int n;
    sink.buf = str;
    sink.pos = 0;
    sink.cap = size;
    n = _akari_format_core(&sink, _sink_putc, NULL, fmt, ap);
    if (str && size > 0) {
        if ((size_t)n < size) str[n] = '\0';
        else str[size-1] = '\0';
    }
    return n;
}

int vsprintf(char *str, const char *fmt, va_list ap)
{
    return vsnprintf(str, (size_t)-1, fmt, ap);
}

int snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap; int n;
    va_start(ap, fmt);
    n = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *str, const char *fmt, ...)
{
    va_list ap; int n;
    va_start(ap, fmt);
    n = vsnprintf(str, (size_t)-1, fmt, ap);
    va_end(ap);
    return n;
}

int _akari_vformat(char *buf, size_t size, const char *fmt, va_list ap)
{ return vsnprintf(buf, size, fmt, ap); }
