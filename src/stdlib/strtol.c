/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * strtol.c -- numeric string conversion (atoi, atol, strtol, strtoul, atof/strtod stub).
 */
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

static int _is_digit(int c, int base)
{
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
    else return -1;
    return d < base ? d : -1;
}

long strtol(const char *s, char **endptr, int base)
{
    long acc = 0;
    int neg = 0;
    int any = 0;
    const char *start = s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+') { s++; }
    else if (*s == '-') { neg = 1; s++; }
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        base = 16;
    } else if (base == 0 && s[0] == '0') {
        s++;
        base = 8;
    } else if (base == 0) {
        base = 10;
    }
    for (;;) {
        int d = _is_digit((unsigned char)*s, base);
        if (d < 0) break;
        if (acc > (LONG_MAX - d) / base) {
            /* overflow */
            acc = neg ? LONG_MIN : LONG_MAX;
            errno = ERANGE;
            any = 1;
            /* consume remaining digits */
            s++;
            while (_is_digit((unsigned char)*s, base) >= 0) s++;
            break;
        }
        acc = acc * base + d;
        any = 1;
        s++;
    }
    if (endptr) *endptr = (char *)(any ? s : start);
    return neg ? -acc : acc;
}

unsigned long strtoul(const char *s, char **endptr, int base)
{
    unsigned long acc = 0;
    int neg = 0;
    int any = 0;
    const char *start = s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+') { s++; }
    else if (*s == '-') { neg = 1; s++; }
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2; base = 16;
    } else if (base == 0 && s[0] == '0') { s++; base = 8; }
    else if (base == 0) base = 10;
    for (;;) {
        int d = _is_digit((unsigned char)*s, base);
        if (d < 0) break;
        if (acc > (ULONG_MAX - (unsigned long)d) / (unsigned long)base) {
            acc = ULONG_MAX; errno = ERANGE; any = 1;
            s++;
            while (_is_digit((unsigned char)*s, base) >= 0) s++;
            break;
        }
        acc = acc * (unsigned long)base + (unsigned long)d;
        any = 1; s++;
    }
    if (endptr) *endptr = (char *)(any ? s : start);
    return neg ? (unsigned long)-(long)acc : acc;
}

int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }

double atof(const char *s) { return strtod(s, NULL); }

double strtod(const char *s, char **endptr)
{
    /* Minimal C-locale strtod; handles signs, decimals, exponents.
     * Uses long double math intermediates to avoid excessive error. */
    double val = 0.0, frac = 0.0, exp = 0.0;
    int neg = 0, eneg = 0, any = 0;
    const char *start = s;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '+') { s++; } else if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { val = val * 10.0 + (*s - '0'); any = 1; s++; }
    if (*s == '.' ) {
        double div = 10.0;
        s++;
        while (*s >= '0' && *s <= '9') { frac += (*s - '0') / div; div *= 10.0; any = 1; s++; }
    }
    val += frac;
    if (any && (*s == 'e' || *s == 'E')) {
        s++;
        if (*s == '+') s++; else if (*s == '-') { eneg = 1; s++; }
        while (*s >= '0' && *s <= '9') { exp = exp * 10.0 + (*s - '0'); s++; }
        { double m = 1.0, b = 10.0; long e = (long)exp;
          if (eneg) e = -e;
          while (e > 0) { if (e & 1) m *= b; b *= b; e >>= 1; }
          while (e < 0) { if (e & 1) m /= b; b *= b; e++; }
          val *= m;
        }
    }
    if (endptr) *endptr = (char *)(any ? s : start);
    return neg ? -val : val;
}
