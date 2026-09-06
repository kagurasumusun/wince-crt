/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * signal.c -- ANSI signal stubs.  Windows CE does not support POSIX
 * signals; these stubs allow POSIX-leaning code to compile.  raise()
 * calls DebugBreak/abort for SIGABRT/SIGSEGV/SIGILL/SIGFPE and is
 * otherwise a no-op that returns 0.
 */
#include <signal.h>
#include <stddef.h>
#include <akari/compiler.h>

typedef void (*sa_fn)(int);
static sa_fn _handlers[16];

static int _sig_idx(int sig) {
    switch (sig) {
        case SIGINT:  return 0;
        case SIGILL:  return 1;
        case SIGABRT: return 2;
        case SIGFPE:  return 3;
        case SIGSEGV: return 4;
        case SIGTERM: return 5;
        default: return -1;
    }
}

sa_fn signal(int sig, sa_fn func)
{
    int i = _sig_idx(sig);
    if (i < 0) return SIG_ERR;
    sa_fn old = _handlers[i] ? _handlers[i] : SIG_DFL;
    if (func != SIG_DFL && func != SIG_IGN) _handlers[i] = func;
    else _handlers[i] = NULL;
    return old;
}

NORETURN void abort(void);  /* from exit.c */

int raise(int sig)
{
    int i = _sig_idx(sig);
    if (i >= 0 && _handlers[i]) {
        _handlers[i](sig);
        return 0;
    }
    switch (sig) {
        case SIGABRT:
        case SIGILL:
        case SIGFPE:
        case SIGSEGV:
            abort();
            break;
    }
    return 0;
}
