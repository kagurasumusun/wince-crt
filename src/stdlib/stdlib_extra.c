/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void *_recalloc(void *ptr, size_t cnt, size_t sz)
{
    size_t total = cnt * sz;
    void *n = realloc(ptr, total);
    if (n) memset(n, 0, total);
    return n;
}
void *_expand(void *p, size_t n){(void)p;(void)n;return NULL;}
size_t _msize(void *p){(void)p;return 0;}
int _onexit(void (*fn)(void)){ return atexit(fn); }
int putenv(char *s){(void)s;return -1;}
int _cexit(void){return 0;}
int _c_exit(void){return 0;}

long long llabs(long long i){ return i<0?-i:i; }
lldiv_t lldiv(long long n, long long d)
{ lldiv_t r; r.quot=n/d; r.rem=n%d; return r; }

double strtold(const char *s, char **e){return strtod(s,e);}

char *gcvt(double v, int digits, char *b){(void)digits;sprintf(b,"%.10g",v);return b;}
char *ecvt(double v, int d, int *dec, int *sign)
{
    static char buf[32];
    char fmt[16]; sprintf(fmt, "%%.%de", d<1?1:d);
    sprintf(buf, fmt, v);
    /* Parse "1.2345e+02" to digits */
    char *e = buf;
    if (*e == '-') { *sign = 1; e++; } else *sign = 0;
    char *dot = strchr(e,'.');
    char *ep = strchr(e,'e');
    if (!ep) ep = e + strlen(e);
    int exp = 0;
    if (ep) exp = atoi(ep+1);
    static char digits[64];
    int di = 0;
    for (char *p = e; p < ep; p++) if (*p >= '0' && *p <= '9') digits[di++]=*p;
    digits[di] = 0;
    if (dot) *dec = exp + (int)(dot - e); else *dec = exp + di;
    return digits;
}
char *fcvt(double v, int d, int *dec, int *sign){return ecvt(v,d,dec,sign);}
