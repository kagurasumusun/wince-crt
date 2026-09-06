/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * malloc.c -- heap allocation.
 *
 * Real Windows CE backend:   coredll!LocalAlloc / LocalFree.
 * Host-check backend:        trivial bump allocator on a static array,
 *                            used ONLY for self-tests.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <akari/compiler.h>

#ifdef _DEBUG_HOSTCHECK_

   /* Bump allocator for host self-tests.  1 MiB is plenty. */
#  define AKARI_HOST_HEAP_SIZE (1u << 20)
   static unsigned char _akari_host_heap[AKARI_HOST_HEAP_SIZE];
   static size_t _akari_host_heap_used = 0;

   static void *_akari_halloc(size_t n) {
       size_t off = (_akari_host_heap_used + 7u) & ~(size_t)7u; /* align to 8 */
       if (off + n > AKARI_HOST_HEAP_SIZE) return NULL;
       void *p = &_akari_host_heap[off];
       _akari_host_heap_used = off + n;
       return p;
   }
   static void _akari_hfree(void *p) { (void)p; /* leak -- tests are short-lived */ }

#else
#  include <akari/windef.h>
#  include <akari/winnt.h>
   typedef HLOCAL akari_hmem;
   static akari_hmem _akari_halloc(size_t n) { return LocalAlloc(LPTR, (UINT)n); }
   static void       _akari_hfree(akari_hmem p) { LocalFree(p); }
#endif

#define AKARI_MALLOC_ALIGN   8u

typedef struct _akari_mhead {
    void       *hmem;
    size_t      size;
    uint32_t    magic;
} akari_mhead;

#define AKARI_MALLOC_MAGIC  0xA0B1C2D3u

static inline int _akari_ptr_aligned(const void *p, size_t a)
{ return ((uintptr_t)p & (a - 1)) == 0; }

static akari_mhead *_akari_head(void *p)
{ return (akari_mhead *)((unsigned char *)p - sizeof(akari_mhead)); }

void *malloc(size_t size)
{
    void *h;
    unsigned char *payload;
    akari_mhead *head;
    size_t total;
    if (size == 0) size = 1;
    total = size + 16u + sizeof(akari_mhead);
    h = _akari_halloc(total);
    if (h == NULL) { errno = ENOMEM; return NULL; }
    payload = (unsigned char *)h;
    payload += sizeof(akari_mhead);
    if (!_akari_ptr_aligned(payload, AKARI_MALLOC_ALIGN)) {
        payload += AKARI_MALLOC_ALIGN -
                   ((uintptr_t)payload & (AKARI_MALLOC_ALIGN - 1));
    }
    head = (akari_mhead *)(payload - sizeof(akari_mhead));
    head->hmem  = h;
    head->size  = size;
    head->magic = AKARI_MALLOC_MAGIC;
    return payload;
}

void free(void *ptr)
{
    akari_mhead *head;
    if (!ptr) return;
    head = _akari_head(ptr);
    if (head->magic != AKARI_MALLOC_MAGIC) return;
    head->magic = 0;
    _akari_hfree(head->hmem);
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p;
    if (nmemb && size && total / nmemb != size) { errno = ENOMEM; return NULL; }
    p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    akari_mhead *head;
    void *nptr;
    if (ptr == NULL) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    head = _akari_head(ptr);
    if (head->magic != AKARI_MALLOC_MAGIC) { errno = EINVAL; return NULL; }
    nptr = malloc(size);
    if (!nptr) return NULL;
    {
        size_t copysz = head->size < size ? head->size : size;
        memcpy(nptr, ptr, copysz);
    }
    free(ptr);
    return nptr;
}
