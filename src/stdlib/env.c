/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * env.c -- getenv/system stubs.  Windows CE has no shell and no
 * environment variables; both are stubbed so portable code links.
 */
#include <stdlib.h>
#include <stddef.h>

char *getenv(const char *name) { (void)name; return NULL; }
int system(const char *cmd) { (void)cmd; return -1; }
