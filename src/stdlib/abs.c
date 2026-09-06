/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stdlib.h>

int abs(int i) { return i < 0 ? -i : i; }
long labs(long i) { return i < 0 ? -i : i; }

div_t div(int num, int denom)
{
    div_t r;
    r.quot = num / denom;
    r.rem  = num % denom;
    return r;
}
ldiv_t ldiv(long num, long denom)
{
    ldiv_t r;
    r.quot = num / denom;
    r.rem  = num % denom;
    return r;
}
