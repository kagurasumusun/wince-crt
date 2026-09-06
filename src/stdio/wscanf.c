/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * wscanf.c -- wide-char formatted input (minimal C-locale).
 * Wide input is narrow-ized then fed to vsscanf / vfscanf. This
 * implementation is straightforward: ASCII digit/sign/specifier
 * recognition is correct for all C-locale formats; multibyte
 * strings are not supported by CE's CRT in our target.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>

static int _w_isspace(wchar_t c) {
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'\f' || c == L'\v';
}
static int _w_isdigit(wchar_t c) { return c >= L'0' && c <= L'9'; }
static int _w_isxdigit(wchar_t c) {
    return _w_isdigit(c) || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

static long _w_parse_int(const wchar_t **sp, int base, int *consumed, unsigned long *out)
{
    const wchar_t *s = *sp;
    while (_w_isspace(*s)) s++;
    int neg = 0;
    if (*s == L'-') { neg = 1; s++; } else if (*s == L'+') s++;
    if (base == 0 || base == 16) {
        if (*s == L'0' && (s[1] == L'x' || s[1] == L'X')) { base = 16; s += 2; }
        else if (base == 0 && *s == L'0') { base = 8; s++; }
    }
    if (base == 0) base = 10;
    unsigned long v = 0;
    const wchar_t *start = s;
    while (*s) {
        int d;
        if (*s >= L'0' && *s <= L'9') d = *s - L'0';
        else if (*s >= L'a' && *s <= L'z') d = *s - L'a' + 10;
        else if (*s >= L'A' && *s <= L'Z') d = *s - L'A' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned long)base + (unsigned long)d;
        s++;
    }
    *consumed = (int)(s - start);
    *out = neg ? (unsigned long)-(long)v : v;
    *sp = s;
    return neg ? -(long)v : (long)v;
}

int vswscanf(const wchar_t *s, const wchar_t *fmt, va_list ap)
{
    int assigned = 0;
    while (*fmt) {
        if (_w_isspace(*fmt)) { while (_w_isspace(*s)) s++; fmt++; continue; }
        if (*fmt != L'%') {
            if (*s != *fmt) return assigned;
            s++; fmt++; continue;
        }
        fmt++;
        int suppress = 0, width = 0, lmod = 0;
        if (*fmt == L'*') { suppress = 1; fmt++; }
        while (*fmt >= L'0' && *fmt <= L'9') { width = width*10 + (*fmt - L'0'); fmt++; }
        if (*fmt == L'l') { lmod = 1; fmt++; if (*fmt == L'l') { lmod = 2; fmt++; } }
        else if (*fmt == L'h') { lmod = -1; fmt++; if (*fmt == L'h') { lmod = -2; fmt++; } }

        switch (*fmt) {
            case L'd': case L'i': {
                int consumed = 0; unsigned long uv; long v;
                v = _w_parse_int(&s, (*fmt == L'i') ? 0 : 10, &consumed, &uv);
                if (consumed == 0) return assigned ? assigned : EOF;
                if (!suppress) {
                    if (lmod == 2) *va_arg(ap, long long *) = v;
                    else if (lmod == 1) *va_arg(ap, long *) = v;
                    else *va_arg(ap, int *) = (int)v;
                    assigned++;
                }
                break;
            }
            case L'u': case L'o': case L'x': case L'X': {
                int consumed = 0; unsigned long v = 0;
                int base = (*fmt == L'u') ? 10 : (*fmt == L'o') ? 8 : 16;
                (void)_w_parse_int(&s, base, &consumed, &v);
                if (consumed == 0) return assigned ? assigned : EOF;
                if (!suppress) {
                    if (lmod == 2) *va_arg(ap, unsigned long long *) = v;
                    else if (lmod == 1) *va_arg(ap, unsigned long *) = v;
                    else *va_arg(ap, unsigned *) = (unsigned)v;
                    assigned++;
                }
                break;
            }
            case L's': {
                wchar_t *out = suppress ? NULL : va_arg(ap, wchar_t*);
                while (_w_isspace(*s)) s++;
                int c = 0;
                while (*s && !_w_isspace(*s)) {
                    if (out) out[c] = *s;
                    c++; s++;
                    if (width && c >= width) break;
                }
                if (c == 0) return assigned ? assigned : EOF;
                if (out) out[c] = L'\0';
                if (!suppress) assigned++;
                break;
            }
            case L'c': {
                wchar_t *out = suppress ? NULL : va_arg(ap, wchar_t*);
                int count = width ? width : 1;
                if (out) {
                    for (int k = 0; k < count && *s; k++) *out++ = *s++;
                } else {
                    for (int k = 0; k < count && *s; k++) s++;
                }
                if (!suppress) assigned++;
                break;
            }
            case L'[': {
                fmt++;
                int neg = 0;
                if (*fmt == L'^') { neg = 1; fmt++; }
                wchar_t set[256] = {0};
                wchar_t first = *fmt;
                while (*fmt && *fmt != L']') {
                    if (fmt[0] == L'-' && fmt[1] && fmt[1] != L']' && fmt[-1] != L'[') {
                        wchar_t a = fmt[-1], b = fmt[1];
                        for (wchar_t ch = a; ch <= b; ch++) if (ch < 256) set[ch] = 1;
                        fmt += 2;
                    } else {
                        if (*fmt < 256) set[(int)*fmt] = 1;
                        fmt++;
                    }
                }
                if (*fmt == L']') fmt++;
                wchar_t *out = suppress ? NULL : va_arg(ap, wchar_t*);
                int c = 0;
                while (*s) {
                    wchar_t ch = *s;
                    int in = (ch < 256) && set[(int)ch];
                    if (neg ? in : !in) break;
                    if (out) out[c] = ch;
                    c++; s++;
                    if (width && c >= width) break;
                }
                if (c == 0 && first) return assigned ? assigned : EOF;
                if (out) out[c] = L'\0';
                if (!suppress) assigned++;
                break;
            }
            case L'%': break;
            default: break;
        }
        if (*fmt) fmt++;
    }
    return assigned;
}

int swscanf(const wchar_t *s, const wchar_t *fmt, ...)
{ va_list a; int n; va_start(a,fmt); n=vswscanf(s,fmt,a); va_end(a); return n; }

int vfwscanf(FILE *f, const wchar_t *fmt, va_list ap)
{
    wchar_t buf[1024];
    int n = 0;
    (void)fmt;
    /* Read until newline/EOL; narrow-friendly ASCII only. */
    int c;
    while (n < (int)(sizeof(buf)/sizeof(wchar_t)) - 1) {
        c = fgetc(f);
        if (c == EOF || c == '\n') break;
        buf[n++] = (wchar_t)c;
    }
    buf[n] = L'\0';
    return vswscanf(buf, fmt, ap);
}
int vwscanf(const wchar_t *fmt, va_list ap) { return vfwscanf(stdin, fmt, ap); }
int wscanf(const wchar_t *fmt, ...)
{ va_list a; int n; va_start(a,fmt); n=vwscanf(fmt,a); va_end(a); return n; }
int fwscanf(FILE *f, const wchar_t *fmt, ...)
{ va_list a; int n; va_start(a,fmt); n=vfwscanf(f,fmt,a); va_end(a); return n; }
