/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void *lfind(const void *key, const void *base, size_t *nmemb, size_t size,
            int (*cmp)(const void*, const void*))
{
    const char *b = (const char *)base;
    size_t i;
    for (i = 0; i < *nmemb; i++) {
        if (cmp(key, b + i*size) == 0) return (void*)(b + i*size);
    }
    return NULL;
}

void *lsearch(const void *key, void *base, size_t *nmemb, size_t size,
              int (*cmp)(const void*, const void*))
{
    void *r = lfind(key, base, nmemb, size, cmp);
    if (r) return r;
    char *b = (char *)base;
    memcpy(b + (*nmemb)*size, key, size);
    (*nmemb)++;
    return b + (*nmemb - 1)*size;
}

/* Minimal hash table for hsearch */
#define _HTAB_SZ 64
static ENTRY *_akari_htab[_HTAB_SZ];
static int _akari_htab_inited = 0;

static unsigned _hash(const char *s)
{
    unsigned h = 2166136261U;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619U; }
    return h % _HTAB_SZ;
}

int hcreate(size_t n){(void)n; if(!_akari_htab_inited){for(int i=0;i<_HTAB_SZ;i++)_akari_htab[i]=NULL; _akari_htab_inited=1;} return 1;}
void hdestroy(void){for(int i=0;i<_HTAB_SZ;i++)_akari_htab[i]=NULL;}
static ENTRY _akari_retentry;
ENTRY *hsearch(ENTRY item, ACTION action)
{
    unsigned h = _hash(item.key);
    for (unsigned i = 0; i < _HTAB_SZ; i++) {
        unsigned idx = (h + i) % _HTAB_SZ;
        if (!_akari_htab[idx]) {
            if (action == ENTER) {
                _akari_htab[idx] = &_akari_retentry;
                _akari_retentry = item;
                return &_akari_retentry;
            }
            return NULL;
        }
        if (strcmp(_akari_htab[idx]->key, item.key) == 0) return _akari_htab[idx];
    }
    return NULL;
}
