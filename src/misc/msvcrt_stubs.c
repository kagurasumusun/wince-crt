/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * msvcrt_stubs.c -- Small MSVCRT-compatibility helpers that don't
 * justify their own file: _fptostr, _ftol, _locking, _remove.
 */
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <akari/compiler.h>

int remove(const char *p);

/* _fptostr / _ftol were exported by old msvcrt for internal printf use.
 * _ftol converts a double to a long (truncate toward zero). _fptostr is
 * a no-op helper for float-to-string printing; our printf doesn't need
 * it but we provide it for ABI compatibility. */
long _ftol(double x) { return (long)x; }
double _fptostr(double x) { return x; }

int _locking(int fd, int mode, long n) { (void)fd;(void)mode;(void)n; return -1; }
int _remove(const char *p) { return remove(p); }
