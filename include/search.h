/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_SEARCH_H_
#define _AKARI_SEARCH_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *lfind(const void *key, const void *base, size_t *nmemb, size_t size,
            int (*cmp)(const void*, const void*));
void *lsearch(const void *key, void *base, size_t *nmemb, size_t size,
              int (*cmp)(const void*, const void*));

typedef struct entry_t {
    char *key;
    void *data;
} ENTRY;
typedef enum { FIND, ENTER } ACTION;
int hcreate(size_t n);
void hdestroy(void);
ENTRY *hsearch(ENTRY item, ACTION action);

#ifdef __cplusplus
}
#endif
#endif
