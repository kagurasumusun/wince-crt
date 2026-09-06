/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * exit.c -- exit(), _exit(), _Exit(), abort().
 * All terminate via coredll!ExitProcess; exit() also runs atexit
 * handlers registered via _akari_atexit_fini. No stdio finalisation
 * happens here (that is a libc concern).
 */
#include <akari/compiler.h>

/* coredll!ExitProcess */
__declspec(dllimport) void ExitProcess(unsigned int);

void _akari_atexit_fini(void);

NORETURN void _exit(int code)    { ExitProcess((unsigned int)code); for(;;){} }
NORETURN void _Exit(int code)    { _exit(code); }
NORETURN void abort(void)        { _exit(3); }

void _akari_cexit(int code)      { _akari_atexit_fini(); _exit(code); }

NORETURN void exit(int code)     { _akari_cexit(code); for(;;){} }
