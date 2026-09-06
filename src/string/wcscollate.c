/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * Wide-character collation / transformation (C-locale only).
 */
#include <wchar.h>
#include <string.h>
#include <stddef.h>

int wcscoll(const wchar_t *a, const wchar_t *b) { return wcscmp(a, b); }
/* wcsncasecmp is in wcs.c */

size_t wcsxfrm(wchar_t *dest, const wchar_t *src, size_t n)
{
    size_t slen = wcslen(src);
    if (n) {
        size_t cp = (slen >= n) ? n - 1 : slen;
        wmemcpy(dest, src, cp);
        dest[cp] = L'\0';
    }
    return slen;
}

wchar_t *wcpcpy(wchar_t *d, const wchar_t *s)
{
    while ((*d++ = *s++)) {}
    return d - 1;
}
wchar_t *wcpncpy(wchar_t *d, const wchar_t *s, size_t n)
{
    wchar_t *orig = d;
    while (n && (*d = *s)) { d++; s++; n--; }
    if (n) while (n--) *d++ = L'\0';
    return orig + (n ? 0 : (d - orig));
}
