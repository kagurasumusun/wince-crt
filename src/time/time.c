/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * time.c -- minimal time support (clock, time, difftime).
 */
#include <stddef.h>
#include <time.h>
#include <akari/compiler.h>

#ifndef _DEBUG_HOSTCHECK_
#  include <akari/winnt.h>
#endif

clock_t clock(void)
{
#ifdef _DEBUG_HOSTCHECK_
    /* No Win32 on host; return 0 -- tests don't use clock(). */
    return (clock_t)0;
#else
    return (clock_t)GetTickCount();
#endif
}

#define CLOCKS_PER_SEC 1000

time_t time(time_t *t)
{
#ifdef _DEBUG_HOSTCHECK_
    time_t now = (time_t)0;
#else
    /* Best effort: GetTickCount-based uptime; not epoch time. */
    time_t now = (time_t)(GetTickCount() / 1000);
#endif
    if (t) *t = now;
    return now;
}

double difftime(time_t a, time_t b) { return (double)(a - b); }

struct tm *localtime(const time_t *t) { (void)t; return NULL; }
struct tm *gmtime(const time_t *t)    { (void)t; return NULL; }
time_t     mktime(struct tm *tm)      { (void)tm; return (time_t)-1; }
char      *ctime(const time_t *t)     { (void)t; return NULL; }
char      *asctime(const struct tm *tm){ (void)tm; return NULL; }
size_t     strftime(char *s, size_t m, const char *f, const struct tm *tm)
    { (void)s; (void)m; (void)f; (void)tm; return 0; }
