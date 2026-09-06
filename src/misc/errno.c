/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * errno.c -- thread-safe errno.
 *
 * On real Windows CE we use TlsAlloc/TlsGetValue to hold a per-thread
 * pointer. When built for host-side testing, a simple static int is
 * used.
 */
#include <stddef.h>
#include <akari/compiler.h>

#ifdef _DEBUG_HOSTCHECK_
static int _akari_errno_val;
void _akari_errno_init(void) { _akari_errno_val = 0; }
int *_akari_errno_location(void) { return &_akari_errno_val; }
#else
#  include <akari/windef.h>
#  include <akari/winnt.h>
static DWORD _akari_errno_tls = TLS_OUT_OF_INDEXES;

void _akari_errno_init(void)
{
    if (_akari_errno_tls == TLS_OUT_OF_INDEXES) {
        _akari_errno_tls = TlsAlloc();
    }
}

int *_akari_errno_location(void)
{
    static int _akari_errno_fallback = 0;
    int *p;
    if (_akari_errno_tls == TLS_OUT_OF_INDEXES) {
        _akari_errno_init();
        if (_akari_errno_tls == TLS_OUT_OF_INDEXES) return &_akari_errno_fallback;
    }
    p = (int *)TlsGetValue(_akari_errno_tls);
    if (p == NULL) {
        p = (int *)LocalAlloc(LPTR, sizeof(int));
        if (p == NULL) return &_akari_errno_fallback;
        TlsSetValue(_akari_errno_tls, p);
    }
    return p;
}
#endif
