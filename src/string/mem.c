/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * mem.c -- memory operations (memcpy, memmove, memset, memcmp, memchr).
 *
 * Implemented in standard C. No external code used. These functions must
 * not rely on memcpy/memset themselves as the compiler may lower struct
 * copies or bulk initializations to calls to these very symbols.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dest;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned char v = (unsigned char)c;
    while (n && ((uintptr_t)p & 3u)) { *p++ = v; n--; }
    if (n >= 4) {
        uint32_t w = (uint32_t)v | ((uint32_t)v << 8) |
                     ((uint32_t)v << 16) | ((uint32_t)v << 24);
        uint32_t *wp = (uint32_t *)(void *)p;
        while (n >= 4) { *wp++ = w; n -= 4; }
        p = (unsigned char *)wp;
    }
    while (n--) *p++ = v;
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--) {
        if (*a != *b) return (int)*a - (int)*b;
        a++; b++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char v = (unsigned char)c;
    while (n--) {
        if (*p == v) return (void *)(uintptr_t)p;
        p++;
    }
    return NULL;
}

void *memmem(const void *hay, size_t hlen, const void *needle, size_t nlen)
{
    const unsigned char *h = (const unsigned char *)hay;
    const unsigned char *n = (const unsigned char *)needle;
    if (nlen == 0) return (void *)h;
    if (nlen > hlen) return NULL;
    for (size_t i = 0; i + nlen <= hlen; i++) {
        if (h[i] == n[0] && !memcmp(h + i, n, nlen)) return (void *)(h + i);
    }
    return NULL;
}
