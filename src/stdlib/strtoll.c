/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

static int _d(int c, int base) {
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
    else return -1;
    return d < base ? d : -1;
}

long long strtoll(const char *s, char **endptr, int base)
{
    long long acc = 0;
    int neg = 0, any = 0;
    const char *start = s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+') s++;
    else if (*s == '-') { neg = 1; s++; }
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2; base = 16;
    } else if (base == 0 && s[0] == '0') { s++; base = 8; }
    else if (base == 0) base = 10;
    for (;;) {
        int d = _d((unsigned char)*s, base);
        if (d < 0) break;
        if (acc > (LLONG_MAX - d) / base) {
            acc = neg ? LLONG_MIN : LLONG_MAX;
            errno = ERANGE; any = 1; s++;
            while (_d((unsigned char)*s, base) >= 0) s++;
            break;
        }
        acc = acc * base + d; any = 1; s++;
    }
    if (endptr) *endptr = (char *)(any ? s : start);
    return neg ? -acc : acc;
}

unsigned long long strtoull(const char *s, char **endptr, int base)
{
    unsigned long long acc = 0;
    int neg = 0, any = 0;
    const char *start = s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+') s++;
    else if (*s == '-') { neg = 1; s++; }
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2; base = 16;
    } else if (base == 0 && s[0] == '0') { s++; base = 8; }
    else if (base == 0) base = 10;
    for (;;) {
        int d = _d((unsigned char)*s, base);
        if (d < 0) break;
        if (acc > (ULLONG_MAX - (unsigned)d) / (unsigned)base) {
            acc = ULLONG_MAX; errno = ERANGE; any = 1; s++;
            while (_d((unsigned char)*s, base) >= 0) s++;
            break;
        }
        acc = acc * (unsigned)base + (unsigned)d; any = 1; s++;
    }
    if (endptr) *endptr = (char *)(any ? s : start);
    return neg ? 0ULL - acc : acc;
}

long long atoll(const char *s) { return strtoll(s, NULL, 10); }
float strtof(const char *s, char **endptr) { return (float)strtod(s, endptr); }
