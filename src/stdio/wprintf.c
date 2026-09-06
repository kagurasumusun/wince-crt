/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * wprintf.c -- wide-char formatted output (minimal C-locale).
 * Wide characters are converted to narrow via ISO-8859-1 and fed to
 * the same backend as narrow printf.  swprintf/vswprintf write into
 * wide buffers.
 */
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <akari/compiler.h>

/* narrow snprintf we forward to */
extern int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

/* Re-encode narrow -> wide into buffer.  Returns wide chars written. */
static int _narrow_to_wide(wchar_t *out, size_t outsz,
                           const char *src, size_t srclen)
{
    size_t i;
    size_t nmax = outsz ? outsz - 1 : 0;
    if (srclen < nmax) nmax = srclen;
    for (i = 0; i < nmax; i++) out[i] = (wchar_t)(unsigned char)src[i];
    if (outsz) out[i] = L'\0';
    return (int)i;
}

int vswprintf(wchar_t *buf, size_t n, const wchar_t *fmt, va_list ap)
{
    (void)fmt;
    /* We support only narrow formats for now: translate wchar fmt
     * to a narrow stack buffer. */
    char nfmt[512];
    size_t i;
    for (i = 0; i < sizeof(nfmt) - 1 && fmt[i]; i++) {
        nfmt[i] = (char)(unsigned char)(fmt[i] > 0xFF ? '?' : fmt[i]);
    }
    nfmt[i] = '\0';
    char tmp[2048];
    int r = vsnprintf(tmp, sizeof(tmp), nfmt, ap);
    if (r < 0) return -1;
    size_t len = (size_t)r;
    int nwritten = _narrow_to_wide(buf, n, tmp, len);
    if ((size_t)r >= n) return -1; /* would overflow */
    return nwritten;
}

int swprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...)
{
    va_list ap; int r;
    va_start(ap, fmt); r = vswprintf(buf, n, fmt, ap); va_end(ap);
    return r;
}

int vfwprintf(FILE *s, const wchar_t *fmt, va_list ap)
{
    char nfmt[512]; size_t i;
    for (i=0; i<sizeof(nfmt)-1 && fmt[i]; i++)
        nfmt[i] = (char)(unsigned char)(fmt[i] > 0xFF ? '?' : fmt[i]);
    nfmt[i] = 0;
    return vfprintf(s, nfmt, ap);
}
int vwprintf(const wchar_t *fmt, va_list ap) { return vfwprintf(stdout, fmt, ap); }
int wprintf(const wchar_t *fmt, ...)
{ va_list a; int n; va_start(a,fmt); n=vwprintf(fmt,a); va_end(a); return n; }
int fwprintf(FILE *s, const wchar_t *fmt, ...)
{ va_list a; int n; va_start(a,fmt); n=vfwprintf(s,fmt,a); va_end(a); return n; }

/* fgetwc / fputwc / fgetws / fputws / getwchar / putwchar provided in stdio.c */
