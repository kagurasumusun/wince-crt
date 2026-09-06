/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * rand.c -- minimal POSIX.1-2008 rand() implementation (LCG).
 */
#include <stdlib.h>

static unsigned long _akari_rand_next = 1;

int rand(void)
{
    _akari_rand_next = _akari_rand_next * 1103515245UL + 12345UL;
    return (int)((_akari_rand_next >> 16) & 0x7FFF);
}

void srand(unsigned int seed)
{
    _akari_rand_next = (unsigned long)seed;
}
