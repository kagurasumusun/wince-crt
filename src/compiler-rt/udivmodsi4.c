/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * udivmodsi4.c -- software 32-bit unsigned division/modulus.
 *
 * On ARMv4/v5 CPUs without hardware divide, clang calls __aeabi_uidivmod
 * or __udivsi3/__umodsi3. We provide these helpers in plain C.
 */
#include <stdint.h>

unsigned int __udivsi3(unsigned int n, unsigned int d)
{
    if (d == 0) return 0;
    unsigned int q = 0, r = 0;
    int i;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q = (q << 1) | 1; }
        else q <<= 1;
    }
    return q;
}

unsigned int __umodsi3(unsigned int n, unsigned int d)
{
    if (d == 0) return 0;
    unsigned int r = 0;
    int i;
    for (i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) r -= d;
    }
    return r;
}

int __divsi3(int n, int d)
{
    int neg = 0;
    unsigned int un, ud, q;
    if (n < 0) { neg ^= 1; un = (unsigned int)(-n); } else un = (unsigned int)n;
    if (d < 0) { neg ^= 1; ud = (unsigned int)(-d); } else ud = (unsigned int)d;
    q = __udivsi3(un, ud);
    return neg ? -(int)q : (int)q;
}

int __modsi3(int n, int d)
{
    int neg = (n < 0);
    unsigned int un, ud, r;
    if (n < 0) un = (unsigned int)(-n); else un = (unsigned int)n;
    if (d < 0) ud = (unsigned int)(-d); else ud = (unsigned int)d;
    r = __umodsi3(un, ud);
    return neg ? -(int)r : (int)r;
}

/* ARM EABI helper: divide in r0, modulus in r1 */
unsigned int __aeabi_uidiv(unsigned int n, unsigned int d) { return __udivsi3(n, d); }
void __aeabi_uidivmod(unsigned int *rp)
{
    unsigned int n = rp[0], d = rp[1];
    rp[0] = __udivsi3(n, d);
    rp[1] = __umodsi3(n, d);
}
void __aeabi_idivmod(int *rp)
{
    int n = rp[0], d = rp[1];
    int q = __divsi3(n, d);
    rp[0] = q;
    rp[1] = n - q*d;
}
int __aeabi_idiv(int n, int d) { return __divsi3(n, d); }
