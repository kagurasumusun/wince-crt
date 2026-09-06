/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * patchables.c -- __imp_ absolute-import pointers used by code that
 * clang/LLVM generates when calling dllimport-declared functions
 * (PE/COFF indirection stubs). The malloc/free targets are resolved
 * by the consumer's C library at link time.
 */
#include <stddef.h>
#include <akari/compiler.h>

extern void *malloc(unsigned long);
extern void  free(void *);

typedef void *(*_PFN_MALLOC)(unsigned long);
typedef void  (*_PFN_FREE)(void *);

_PFN_MALLOC __imp_malloc __attribute__((weak)) = malloc;
_PFN_FREE   __imp_free   __attribute__((weak)) = free;
