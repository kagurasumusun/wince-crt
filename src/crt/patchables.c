/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * patchables.c -- function-pointer indirection slots that MSVCRT uses.
 * We provide weak defaults pointing at standard implementations so
 * code compiled against the MS ABI links against Akari.
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

typedef int (*_PFN_ATEXIT)(void (*)(void));
typedef void (*_PFN_EXIT)(int);
typedef void *(*_PFN_MALLOC)(size_t);
typedef void (*_PFN_FREE)(void *);

_PFN_MALLOC __imp_malloc = malloc;
_PFN_FREE   __imp_free   = free;
void *(*__imp_calloc)(size_t,size_t) = calloc;
void *(*__imp_realloc)(void*,size_t) = realloc;
