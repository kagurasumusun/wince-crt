/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * exit.c -- _akari_cexit: internal shutdown helper called by the
 * startup code after the user entry point returns.  It runs atexit
 * handlers registered through this CRT and then calls coredll!
 * ExitProcess.
 *
 * The standard C exit() / _Exit() / abort() / _exit() functions are
 * NOT defined here -- they are provided by whichever C library the
 * consumer links against (llvm-libc, newlib, coredll's msvcrt
 * exports, ...). The startup code must NOT call exit() because that
 * would drag the C library's shutdown path in ahead of time; instead
 * it calls _akari_cexit() directly, which is intentionally a private
 * symbol.
 */
#include <akari/compiler.h>

/* coredll!ExitProcess */
__declspec(dllimport) void ExitProcess(unsigned int);

void _akari_atexit_fini(void);

NORETURN void _akari_cexit(int code)
{
    _akari_atexit_fini();
    ExitProcess((unsigned int)code);
    for (;;) { }
}
