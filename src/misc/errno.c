/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * errno.c -- thread-safe errno location used by code compiled against
 * this CRT. On Windows CE we allocate a per-thread slot with TlsAlloc;
 * the current thread's errno is stored on the process heap on first
 * touch. This matches MSVCRT's behaviour.
 *
 * errno itself is declared by the C library headers (libc concern);
 * we only supply the _errno() accessor function that the msvcrt-ABI
 * macro `#define errno (*_errno())` expects.
 */
#include <stddef.h>
#include <akari/compiler.h>

typedef unsigned long DWORD;
typedef void *LPVOID;
typedef int BOOL;
#define LPTR                 0x0040
#define TLS_OUT_OF_INDEXES   ((DWORD)0xFFFFFFFF)

__declspec(dllimport) DWORD  TlsAlloc(void);
__declspec(dllimport) LPVOID TlsGetValue(DWORD);
__declspec(dllimport) BOOL   TlsSetValue(DWORD, LPVOID);
__declspec(dllimport) LPVOID LocalAlloc(unsigned, unsigned);

static DWORD _akari_errno_tls = TLS_OUT_OF_INDEXES;

void _akari_errno_init(void)
{
    if (_akari_errno_tls == TLS_OUT_OF_INDEXES) {
        _akari_errno_tls = TlsAlloc();
    }
}

int *_akari_errno_location(void)
{
    static int fallback;
    if (_akari_errno_tls == TLS_OUT_OF_INDEXES) return &fallback;
    int *p = (int *)TlsGetValue(_akari_errno_tls);
    if (!p) {
        p = (int *)LocalAlloc(LPTR, sizeof(int));
        if (!p) return &fallback;
        TlsSetValue(_akari_errno_tls, p);
    }
    return p;
}

/* MSVCRT ABI name */
int *_errno(void) { return _akari_errno_location(); }
int *__errno(void) { return _akari_errno_location(); }
