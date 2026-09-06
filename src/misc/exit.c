/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * exit.c -- exit() and abort().
 */
#include <stdlib.h>
#include <akari/compiler.h>

void _akari_atexit_fini(void);
void _akari_stdio_fini(void);

#ifdef _DEBUG_HOSTCHECK_
/*
 * Host-side exit routines bypass libc to avoid FILE* conflicts.
 * We use inline assembly to invoke the Linux exit_group syscall.
 */
#  if defined(__x86_64__)
static NORETURN void _akari_host_exit(int code) {
    int c = code;
    __asm__ __volatile__ ("movq $231, %%rax\n\t"
                          "movl %0, %%edi\n\t"
                          "syscall" : : "r"(c) : "rax", "rdi", "rcx", "r11", "memory");
    for (;;) { __asm__ __volatile__ ("":::"memory"); }
}
#  elif defined(__i386__)
static NORETURN void _akari_host_exit(int code) {
    __asm__ __volatile__ ("mov $252, %%eax\n\t"    /* exit_group */
                          "mov %0, %%ebx\n\t"
                          "int $0x80" : : "r"(code) : "memory");
    for (;;) { __asm__ __volatile__ ("":::"memory"); }
}
#  else
static NORETURN void _akari_host_exit(int code) {
    (void)code;
    /* Give up -- infinite loop */
    for (;;) { __asm__ __volatile__ ("":::"memory"); }
}
#  endif

NORETURN void exit(int status)     { _akari_atexit_fini(); _akari_stdio_fini(); _akari_host_exit(status); }
NORETURN void _exit(int status)    { _akari_host_exit(status); }
NORETURN void _Exit(int status)    { _akari_host_exit(status); }
NORETURN void abort(void)          { _akari_host_exit(3); }

#else
#  include <akari/windef.h>
#  include <akari/winnt.h>

NORETURN void exit(int status)
{
    _akari_atexit_fini();
    _akari_stdio_fini();
    ExitProcess((UINT)status);
    for (;;) { }
}

NORETURN void _exit(int status)
{
    ExitProcess((UINT)status);
    for (;;) { }
}

NORETURN void _Exit(int status)
{
    ExitProcess((UINT)status);
    for (;;) { }
}

NORETURN void abort(void)
{
    ExitProcess((UINT)3);
    for (;;) { }
}
#endif
