/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * qsort.c -- quicksort / bsearch implementation.
 *
 * Bentley-McIlroy-style partitioning simplified. No external code used.
 */
#include <stdlib.h>
#include <string.h>

typedef int (*cmp_t)(const void *, const void *);

static void _swap(char *a, char *b, size_t n)
{
    while (n--) { char t = *a; *a++ = *b; *b++ = t; }
}

static char *_med3(char *a, char *b, char *c, cmp_t cmp)
{
    return cmp(a, b) < 0
        ? (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
        : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

static void _insort(char *base, size_t n, size_t sz, cmp_t cmp)
{
    size_t i;
    for (i = 1; i < n; i++) {
        size_t j = i;
        while (j > 0 && cmp(base + (j-1)*sz, base + j*sz) > 0) {
            _swap(base + (j-1)*sz, base + j*sz, sz);
            j--;
        }
    }
}

static void _qsort(char *base, size_t n, size_t sz, cmp_t cmp)
{
    while (n > 1) {
        char *pi, *pj, *pn;
        char *pivot;
        if (n < 7) { _insort(base, n, sz, cmp); return; }
        pivot = _med3(base, base + (n/2)*sz, base + (n-1)*sz, cmp);
        _swap(base, pivot, sz);
        pi = base;
        pj = pn = base + n*sz;
        for (;;) {
            do { pi += sz; } while (pi < pj && cmp(pi, base) < 0);
            do { pj -= sz; } while (pj > base && cmp(pj, base) > 0);
            if (pi >= pj) break;
            _swap(pi, pj, sz);
            if (cmp(pi, base) == 0) { pn -= sz; _swap(pi, pn, sz); }
            if (cmp(pj, base) == 0) { pn -= sz; _swap(pj, pn, sz); }
        }
        _swap(base, pj, sz);
        pj = pj - sz;
        /* Recurse on smaller partition, iterate on larger */
        {
            size_t nl = (size_t)(pj - base) / sz;
            size_t nr = n - (size_t)(pi - base)/sz - 1;
            if (nl < nr) {
                _qsort(base, nl, sz, cmp);
                base = pi;
                n = nr;
            } else {
                _qsort(pi, nr, sz, cmp);
                n = nl;
            }
        }
    }
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    if (nmemb > 1 && size > 0) _qsort((char *)base, nmemb, size, compar);
}
