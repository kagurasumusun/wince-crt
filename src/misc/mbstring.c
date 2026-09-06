/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * mbstring.c -- multi-byte <-> wide conversions for the C locale (ASCII only).
 * On Windows CE the system code page is irrelevant because Windows CE is
 * Unicode-native; we treat the execution character set as UTF-8 for
 * portability, but since embedded targets typically use single-byte chars,
 * we implement a very simple 1:1 mapping (Latin-1) for values < 256.
 * Proper UTF-8 conversion can be added later if needed.
 */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

int mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    if (s == NULL) return 0;
    if (n == 0) return -1;
    if (pwc) *pwc = (wchar_t)(unsigned char)*s;
    return (*s == '\0') ? 0 : 1;
}

int wctomb(char *s, wchar_t wchar)
{
    if (s == NULL) return 0;
    if (wchar > 255) return -1;
    *s = (char)(unsigned char)wchar;
    return (wchar == L'\0') ? 1 : 1;
}

int mblen(const char *s, size_t n) { return mbtowc(NULL, s, n); }

size_t mbstowcs(wchar_t *dst, const char *src, size_t len)
{
    size_t i = 0;
    if (!dst) return strlen(src);
    while (i < len) {
        dst[i] = (wchar_t)(unsigned char)src[i];
        if (src[i] == '\0') return i;
        i++;
    }
    return i;
}

size_t wcstombs(char *dst, const wchar_t *src, size_t len)
{
    size_t i = 0;
    if (!dst) { size_t n = 0; while (src[n]) n++; return n; }
    while (i < len) {
        if (src[i] > 255) { dst[i] = '?'; } else dst[i] = (char)(unsigned char)src[i];
        if (src[i] == L'\0') return i;
        i++;
    }
    return i;
}
