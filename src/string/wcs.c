/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * wcs.c -- wide (UTF-16) string functions.
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

size_t wcslen(const wchar_t *s)
{
    const wchar_t *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

wchar_t *wcscpy(wchar_t *dest, const wchar_t *src)
{
    wchar_t *d = dest;
    while ((*d++ = *src++) != L'\0') {}
    return dest;
}

wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    while (n && (*d = *src)) { d++; src++; n--; }
    while (n--) *d++ = L'\0';
    return dest;
}

wchar_t *wcscat(wchar_t *dest, const wchar_t *src)
{
    wchar_t *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != L'\0') {}
    return dest;
}

wchar_t *wcsncat(wchar_t *dest, const wchar_t *src, size_t n)
{
    wchar_t *d = dest;
    while (*d) d++;
    while (n-- && (*d = *src)) { d++; src++; }
    *d = L'\0';
    return dest;
}

int wcscmp(const wchar_t *a, const wchar_t *b)
{
    while (*a == *b && *a) { a++; b++; }
    return (int)*a - (int)*b;
}

int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n)
{
    while (n && *a == *b && *a) { a++; b++; n--; }
    if (n == 0) return 0;
    return (int)*a - (int)*b;
}

int wcscasecmp(const wchar_t *a, const wchar_t *b)
{
    wchar_t ca, cb;
    do {
        ca = *a; cb = *b;
        if (ca >= L'A' && ca <= L'Z') ca += (wchar_t)(L'a'-L'A');
        if (cb >= L'A' && cb <= L'Z') cb += (wchar_t)(L'a'-L'A');
        a++; b++;
    } while (ca == cb && ca);
    return (int)ca - (int)cb;
}

wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
    for (;;) {
        if (*s == c) return (wchar_t *)s;
        if (*s == L'\0') return NULL;
        s++;
    }
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t c)
{
    const wchar_t *last = NULL;
    do {
        if (*s == c) last = s;
    } while (*s++);
    return (wchar_t *)last;
}

wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept)
{
    const wchar_t *a;
    while (*s) {
        for (a = accept; *a; a++) if (*a == *s) return (wchar_t *)s;
        s++;
    }
    return NULL;
}

size_t wcsspn(const wchar_t *s, const wchar_t *accept)
{
    size_t n = 0;
    const wchar_t *a;
    while (*s) {
        for (a = accept; *a; a++) if (*a == *s) break;
        if (!*a) return n;
        s++; n++;
    }
    return n;
}

size_t wcscspn(const wchar_t *s, const wchar_t *reject)
{
    size_t n = 0;
    const wchar_t *r;
    while (*s) {
        for (r = reject; *r; r++) if (*r == *s) return n;
        s++; n++;
    }
    return n;
}

wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle)
{
    size_t n = wcslen(needle);
    if (n == 0) return (wchar_t *)haystack;
    while (*haystack) {
        if (*haystack == *needle && wcsncmp(haystack, needle, n) == 0)
            return (wchar_t *)haystack;
        haystack++;
    }
    return NULL;
}

wchar_t *wcstok(wchar_t *str, const wchar_t *delim, wchar_t **saveptr)
{
    wchar_t *s;
    if (str == NULL) str = *saveptr;
    if (str == NULL) return NULL;
    s = str + wcsspn(str, delim);
    if (*s == L'\0') { *saveptr = s; return NULL; }
    {
        size_t n = wcscspn(s, delim);
        if (s[n] == L'\0') *saveptr = s + n;
        else { s[n] = L'\0'; *saveptr = s + n + 1; }
    }
    return s;
}

wchar_t *wcsdup(const wchar_t *s)
{
    size_t n = (wcslen(s) + 1) * sizeof(wchar_t);
    wchar_t *r = (wchar_t *)malloc(n);
    if (!r) return NULL;
    memcpy(r, s, n);
    return r;
}
wchar_t *_wcsdup(const wchar_t *s) { return wcsdup(s); }

wchar_t *wmemcpy(wchar_t *d, const wchar_t *s, size_t n) {
    return (wchar_t *)memcpy(d, s, n * sizeof(wchar_t));
}
wchar_t *wmemmove(wchar_t *d, const wchar_t *s, size_t n) {
    return (wchar_t *)memmove(d, s, n * sizeof(wchar_t));
}
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n) {
    wchar_t *p = s;
    while (n--) *p++ = c;
    return s;
}
int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n) {
    while (n--) {
        if (*a != *b) return (int)*a - (int)*b;
        a++; b++;
    }
    return 0;
}
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n) {
    while (n--) {
        if (*s == c) return (wchar_t *)s;
        s++;
    }
    return NULL;
}
size_t wcsnlen(const wchar_t *s, size_t maxlen) {
    size_t n = 0;
    while (n < maxlen && s[n]) n++;
    return n;
}
int wcsncasecmp(const wchar_t *a, const wchar_t *b, size_t n) {
    wchar_t ca, cb;
    while (n--) {
        ca = *a; cb = *b;
        if (ca >= L'A' && ca <= L'Z') ca += (wchar_t)(L'a'-L'A');
        if (cb >= L'A' && cb <= L'Z') cb += (wchar_t)(L'a'-L'A');
        if (ca != cb || ca == 0) return (int)ca - (int)cb;
        a++; b++;
    }
    return 0;
}
