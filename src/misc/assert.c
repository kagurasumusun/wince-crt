/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdio.h>
#include <akari/compiler.h>

NORETURN void _akari_assert_fail(const char *expr, const char *file, int line,
                                const char *func)
{
    char buf[384];
    int n = snprintf(buf, sizeof(buf),
                     "Assertion failed: %s (%s:%d%s%s)\r\n",
                     expr ? expr : "?",
                     file ? file : "?",
                     line,
                     func ? ", " : "",
                     func ? func : "");
    (void)n;
    fputs(buf, stderr);
    fflush(stderr);
#ifdef _DEBUG_HOSTCHECK_
    {
        /* Cause a SIGABRT-style exit: raise(SIGABRT), but use a trap. */
        __asm__ __volatile__ ("int3");
        for (;;) {}
    }
#else
#  include <akari/winnt.h>
    OutputDebugStringA(buf);
    DebugBreak();
    ExitProcess((UINT)3);
    for (;;) {}
#endif
}
