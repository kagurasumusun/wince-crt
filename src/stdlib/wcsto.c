/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * wcsto.c -- wchar_t numeric conversion entry points (wcstod, wcstol,
 * wcstoul, wcstoll, wcstoull). Implementation copies the numeric prefix
 * to a stack narrow buffer and forwards to the existing strto* routines.
 * This is correct for the ASCII/Latin-1 digit/sign/exponent characters
 * that the C standard requires strto* / wcsto* to recognize.
 */
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

/* Copy maximal run of characters valid as a number prefix into buf.
 * Whitespace, sign, digits, hex markers, decimal point, exponent letter,
 * and the 0x/0X/0b/0B prefixes are all ASCII, so copying BMP chars to
 * char is fine (non-ASCII terminates the numeric prefix). */
static size_t _wcp(const wchar_t *s, char *buf, size_t bsz) {
    size_t n = 0;
    int saw_digit = 0;
    int in_exp = 0;
    while (*s && n + 1 < bsz) {
        wchar_t wc = *s;
        if (wc > 0x7F) break;
        char c = (char)wc;
        int ok = 0;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') ok = !saw_digit && !in_exp ? 1 : 0;
        else if (c == '+' || c == '-') ok = (n == 0) || in_exp;
        else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.') ok = 1;
        if (!ok) break;
        buf[n++] = c;
        if (c >= '0' && c <= '9') saw_digit = 1;
        if (c == 'e' || c == 'E' || c == 'p' || c == 'P') in_exp = 1;
        s++;
    }
    buf[n] = '\0';
    return n;
}

double wcstod(const wchar_t *s, wchar_t **endptr) {
    char tmp[128];
    const wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r' || *p == L'\f' || *p == L'\v') p++;
    size_t n = _wcp(p, tmp, sizeof(tmp));
    char *e = NULL;
    double d = strtod(tmp, &e);
    if (endptr) *endptr = (wchar_t*)(e ? p + (e - tmp) : s);
    return d;
}

float wcstof(const wchar_t *s, wchar_t **endptr) { return (float)wcstod(s, endptr); }
long double wcstold(const wchar_t *s, wchar_t **endptr) { return (long double)wcstod(s, endptr); }

long wcstol(const wchar_t *s, wchar_t **endptr, int base) {
    char tmp[128];
    const wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r' || *p == L'\f' || *p == L'\v') p++;
    size_t n = _wcp(p, tmp, sizeof(tmp));
    (void)n;
    char *e = NULL;
    long v = strtol(tmp, &e, base);
    if (endptr) *endptr = (wchar_t*)(e ? p + (e - tmp) : s);
    return v;
}

unsigned long wcstoul(const wchar_t *s, wchar_t **endptr, int base) {
    char tmp[128];
    const wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r' || *p == L'\f' || *p == L'\v') p++;
    _wcp(p, tmp, sizeof(tmp));
    char *e = NULL;
    unsigned long v = strtoul(tmp, &e, base);
    if (endptr) *endptr = (wchar_t*)(e ? p + (e - tmp) : s);
    return v;
}

long long wcstoll(const wchar_t *s, wchar_t **endptr, int base) {
    char tmp[128];
    const wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r' || *p == L'\f' || *p == L'\v') p++;
    _wcp(p, tmp, sizeof(tmp));
    char *e = NULL;
    long long v = strtoll(tmp, &e, base);
    if (endptr) *endptr = (wchar_t*)(e ? p + (e - tmp) : s);
    return v;
}

unsigned long long wcstoull(const wchar_t *s, wchar_t **endptr, int base) {
    char tmp[128];
    const wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'\n' || *p == L'\r' || *p == L'\f' || *p == L'\v') p++;
    _wcp(p, tmp, sizeof(tmp));
    char *e = NULL;
    unsigned long long v = strtoull(tmp, &e, base);
    if (endptr) *endptr = (wchar_t*)(e ? p + (e - tmp) : s);
    return v;
}
