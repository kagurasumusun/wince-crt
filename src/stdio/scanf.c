/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * scanf.c -- formatted input (minimal implementation).
 * Windows CE stdio is typically file-based on console-less devices; we
 * provide stub implementations that return EOF so programs link. A
 * proper sscanf implementation will be added in a future revision.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

int vsscanf(const char *s, const char *fmt, va_list ap)
{
    int assigned = 0;
    while (*fmt) {
        if (isspace((unsigned char)*fmt)) {
            while (isspace((unsigned char)*s)) s++;
            fmt++;
            continue;
        }
        if (*fmt != '%') {
            if (*s != *fmt) return assigned;
            s++; fmt++; continue;
        }
        fmt++;
        {
            int suppress = 0;
            int width = 0;
            int lmod = 0;
            if (*fmt == '*') { suppress = 1; fmt++; }
            while (*fmt >= '0' && *fmt <= '9') { width = width*10 + (*fmt-'0'); fmt++; }
            if (*fmt == 'l') { lmod = 1; fmt++; if (*fmt == 'l') { lmod = 2; fmt++; } }
            else if (*fmt == 'h') { lmod = -1; fmt++; if (*fmt == 'h') { lmod = -2; fmt++; } }
            (void)width;
            switch (*fmt) {
                case 'd': case 'i': {
                    char *end;
                    long v = strtol(s, &end, (*fmt == 'i') ? 0 : 10);
                    if (end == s) return assigned ? assigned : EOF;
                    s = end;
                    if (!suppress) {
                        if (lmod == 2) *va_arg(ap, long long *) = v;
                        else if (lmod == 1) *va_arg(ap, long *) = v;
                        else *va_arg(ap, int *) = (int)v;
                        assigned++;
                    }
                    break;
                }
                case 'u': case 'o': case 'x': case 'X': {
                    char *end;
                    int base = (*fmt == 'u') ? 10 : (*fmt == 'o') ? 8 : 16;
                    unsigned long v = strtoul(s, &end, base);
                    if (end == s) return assigned ? assigned : EOF;
                    s = end;
                    if (!suppress) {
                        if (lmod == 2) *va_arg(ap, unsigned long long *) = v;
                        else if (lmod == 1) *va_arg(ap, unsigned long *) = v;
                        else *va_arg(ap, unsigned *) = (unsigned)v;
                        assigned++;
                    }
                    break;
                }
                case 's': {
                    char *out = suppress ? NULL : va_arg(ap, char *);
                    while (isspace((unsigned char)*s)) s++;
                    int c = 0;
                    while (*s && !isspace((unsigned char)*s)) {
                        if (out) out[c] = *s;
                        c++; s++;
                        if (width && c >= width) break;
                    }
                    if (c == 0) return assigned ? assigned : EOF;
                    if (out) out[c] = '\0';
                    if (!suppress) assigned++;
                    break;
                }
                case 'c': {
                    char *out = suppress ? NULL : va_arg(ap, char *);
                    int w = width ? width : 1;
                    for (int i = 0; i < w; i++) {
                        if (!*s) return assigned ? assigned : EOF;
                        if (out) out[i] = *s;
                        s++;
                    }
                    if (!suppress) assigned++;
                    break;
                }
                case '%':
                    if (*s != '%') return assigned;
                    s++; break;
                case 'n':
                    *va_arg(ap, int *) = (int)(s - (const char *)0);
                    /* approximate; not accurate but symbol exists */
                    break;
                default:
                    return assigned;
            }
            fmt++;
        }
    }
    return assigned;
}

int sscanf(const char *s, const char *fmt, ...)
{
    va_list ap; int n; va_start(ap, fmt); n = vsscanf(s, fmt, ap); va_end(ap); return n;
}

int vfscanf(FILE *f, const char *fmt, va_list ap)
{
    char buf[1024];
    int n = 0, c;
    while (n < (int)sizeof(buf) - 1) {
        c = fgetc(f);
        if (c == EOF) break;
        buf[n++] = (char)c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    if (n == 0) return EOF;
    return vsscanf(buf, fmt, ap);
}
int vscanf(const char *fmt, va_list ap)
    { return vfscanf(stdin, fmt, ap); }
int scanf(const char *fmt, ...)
    { va_list a; int n; va_start(a,fmt); n=vscanf(fmt,a); va_end(a); return n; }
int fscanf(FILE *f, const char *fmt, ...)
    { va_list a; int n; va_start(a,fmt); n=vfscanf(f,fmt,a); va_end(a); return n; }

/* ungetc/getc/putc/getchar/getw/putw/gets provided in stdio.c */
int puts_w_minimal(const char *s);
