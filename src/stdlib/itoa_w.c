/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * itoa_w.c -- wide-string numeric helpers: _wtoi/_wtol/_wtoi64/_wtof.
 * Narrow and wide itoa/ltow/ultow/i64tow/ui64tow are implemented in
 * itoa.c; we only add the wchar->number helpers here.
 */
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

long long _wtoi64(const wchar_t *s) {
    long long v = 0; int neg = 0;
    while (*s == L' ' || *s == L'\t') s++;
    if (*s == L'-') { neg = 1; s++; } else if (*s == L'+') s++;
    while (*s >= L'0' && *s <= L'9') { v = v*10 + (*s - L'0'); s++; }
    return neg ? -v : v;
}
long _wtol(const wchar_t *s) { return (long)_wtoi64(s); }
int _wtoi(const wchar_t *s)  { return (int)_wtoi64(s); }
double _wtof(const wchar_t *s) {
    char buf[64]; int i = 0;
    while (*s && i < (int)sizeof(buf)-1) {
        buf[i++] = (*s < 128) ? (char)*s : '?';
        s++;
    }
    buf[i] = 0;
    return strtod(buf, NULL);
}
