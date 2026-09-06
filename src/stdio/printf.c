/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * printf.c -- printf / sprintf / snprintf formatter engine.
 *
 * Self-contained implementation of a C99-subset printf formatter.
 * Supports:
 *   %d %i %u %o %x %X %c %s %p %f %% %n
 *   flags -, +, space, 0, #, width, precision, *, length modifiers
 *     hh  h  l  ll  z
 */
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "printf_core.h"

#define AKARI_PRINTF_BUF 40

static int _akari_pad(void *ctx, akari_putc_fn putc, akari_puts_fn puts_fn,
                      char c, int n)
{
    char pad[16];
    int total = 0;
    memset(pad, c, sizeof(pad));
    while (n > 0) {
        int chunk = (n > (int)sizeof(pad)) ? (int)sizeof(pad) : n;
        int r = puts_fn ? puts_fn(ctx, pad, chunk) : 0;
        if (!puts_fn) {
            int i;
            for (i = 0; i < chunk; i++) if (putc(ctx, (unsigned char)pad[i]) < 0) return -1;
            r = chunk;
        }
        if (r < 0) return -1;
        total += r; n -= r;
    }
    return total;
}

static int _akari_format_unsigned(uint64_t u, unsigned base, int upper,
                                  char *buf, int bufsize)
{
    static const char digL[] = "0123456789abcdef";
    static const char digU[] = "0123456789ABCDEF";
    const char *dig = upper ? digU : digL;
    int i = bufsize - 1;
    buf[i] = '\0';
    if (u == 0) { buf[--i] = '0'; return i; }
    while (u) { buf[--i] = dig[u % base]; u /= base; }
    return i;
}

static int _akari_format_signed(int64_t v, char *buf, int bufsize)
{
    int neg = 0;
    uint64_t u;
    int i = bufsize - 1;
    buf[i] = '\0';
    if (v < 0) { neg = 1; u = (uint64_t)(-(v + 1)) + 1u; } else { u = (uint64_t)v; }
    if (u == 0) { buf[--i] = '0'; }
    else while (u) { buf[--i] = '0' + (char)(u % 10); u /= 10; }
    if (neg) buf[--i] = '-';
    return i;
}

static int _akari_format_double(double val, char *buf, int bufsize, int prec, char fmt)
{
    char *p = buf;
    char *end = buf + bufsize - 2;
    int neg = 0, i;
    uint64_t ip;
    double frac;
    if (prec < 0) prec = (fmt == 'g' || fmt == 'G') ? 6 : (fmt == 'e' || fmt == 'E') ? 6 : 6;
    if (val != val) { /* NaN */
        const char *nan = (fmt == 'E' || fmt == 'G') ? "NAN" : "nan";
        while (*nan && p < end) *p++ = *nan++;
        *p = '\0'; return 0;
    }
    if (val < 0) { neg = 1; if (p < end) *p++ = '-'; val = -val; }
    if (val == val && (val + val == val) && val != 0.0) { /* inf */
        const char *inf = (fmt == 'E' || fmt == 'G') ? "INF" : "inf";
        while (*inf && p < end) *p++ = *inf++;
        *p = '\0'; return 0;
    }

    if (fmt == 'f' || fmt == 'F') {
        /* simple rounding */
        {
            double mul = 1.0; int pr = prec; while (pr-- > 0) mul *= 10.0; val += 0.5 / mul;
        }
        ip = (uint64_t)val;
        frac = val - (double)ip;
        {
            char tmp[32]; int ti = 31; tmp[ti] = '\0';
            if (ip == 0) { tmp[--ti] = '0'; }
            else { uint64_t u = ip; while (u) { tmp[--ti] = '0' + (char)(u%10); u/=10; } }
            while (tmp[ti] && p < end) *p++ = tmp[ti++];
        }
        if (prec > 0 && p < end) {
            *p++ = '.';
            for (i = 0; i < prec && p < end; i++) {
                frac *= 10.0;
                int d = (int)frac;
                if (d > 9) d = 9;
                if (d < 0) d = 0;
                *p++ = '0' + (char)d;
                frac -= d;
            }
        }
        *p = '\0';
        (void)neg; (void)end;
        return 0;
    }

    /* %e / %E: scientific notation [-]d.ddde±dd */
    if (fmt == 'e' || fmt == 'E') {
        int exp = 0;
        char echar = (fmt == 'E') ? 'E' : 'e';
        if (val != 0.0) {
            while (val >= 10.0) { val /= 10.0; exp++; }
            while (val < 1.0)   { val *= 10.0; exp--; }
        }
        /* rounding to prec digits after decimal */
        double mul = 1.0; int pr = prec; while (pr-- > 0) mul *= 10.0;
        val += 0.5 / mul;
        if (val >= 10.0) { val /= 10.0; exp++; }
        /* integer part */
        int di = (int)val; if (di > 9) di = 9;
        if (p < end) *p++ = '0' + (char)di;
        val -= di;
        if (prec > 0 && p < end) {
            *p++ = '.';
            for (i = 0; i < prec && p < end; i++) {
                val *= 10.0;
                int d = (int)val;
                if (d > 9) d = 9;
                *p++ = '0' + (char)d;
                val -= d;
            }
        }
        if (p < end) *p++ = echar;
        if (p < end) *p++ = (exp < 0) ? '-' : '+';
        if (exp < 0) exp = -exp;
        if (p < end) *p++ = '0' + (char)(exp / 100);
        exp %= 100;
        if (p < end) *p++ = '0' + (char)(exp / 10);
        exp %= 10;
        if (p < end) *p++ = '0' + (char)exp;
        *p = '\0';
        return 0;
    }

    /* %g / %G: use %f or %e depending on exponent; trim trailing zeros. */
    if (fmt == 'g' || fmt == 'G') {
        int efmt_eff = prec ? prec : 1;
        double av = val; if (av < 0) av = -av;
        int use_e = 0; int ex = 0;
        if (av != 0.0) {
            double t = av;
            if (t >= 1.0) { while (t >= 10.0) { t /= 10.0; ex++; } }
            else { while (t < 1.0) { t *= 10.0; ex--; } }
            if (ex < -4 || ex >= efmt_eff) use_e = 1;
        }
        char tmp[64];
        if (use_e) {
            _akari_format_double(val, tmp, sizeof(tmp), prec - 1, fmt == 'G' ? 'E' : 'e');
        } else {
            int gprec = prec - 1 - ex;
            if (gprec < 0) gprec = 0;
            _akari_format_double(val, tmp, sizeof(tmp), gprec, 'f');
        }
        /* trim trailing zeros and possible decimal point */
        int tl = (int)strlen(tmp);
        char *dot = NULL;
        for (i = 0; tmp[i]; i++) if (tmp[i] == '.') { dot = tmp+i; break; }
        if (dot) {
            char *t = tmp + tl;
            while (t > dot+1 && *(t-1) == '0') *--t = '\0';
            if (t == dot+1 && *dot == '.' && t[-1] != '\0') {
                /* leave dot per spec? %g traditionally strips the dot; do so */
                *dot = '\0';
            }
        }
        for (i = 0; tmp[i] && p < end; i++) {
            char c = tmp[i];
            if (fmt == 'G' && c == 'e') c = 'E';
            *p++ = c;
        }
        *p = '\0';
        return 0;
    }
    *p = '\0';
    return 0;
}

int _akari_format_core(void *ctx, akari_putc_fn putc, akari_puts_fn puts_fn,
                       const char *fmt, va_list ap)
{
    int total = 0;
    const char *s;
    for (s = fmt; *s; s++) {
        int add;
        if (*s != '%') {
            if (putc(ctx, (unsigned char)*s) < 0) return -1;
            total++; continue;
        }
        s++;
        int left = 0, zero = 0, plus = 0, sp = 0, alt = 0;
        for (;; s++) {
            if      (*s == '-') left = 1;
            else if (*s == '0') zero = 1;
            else if (*s == '+') plus = 1;
            else if (*s == ' ') sp = 1;
            else if (*s == '#') alt = 1;
            else break;
        }
        int width = 0;
        if (*s == '*') { width = va_arg(ap, int); s++; if (width < 0) { left = 1; width = -width; } }
        else while (*s >= '0' && *s <= '9') { width = width*10 + (*s - '0'); s++; }
        int prec = -1;
        if (*s == '.') {
            s++; prec = 0;
            if (*s == '*') { prec = va_arg(ap, int); s++; }
            else while (*s >= '0' && *s <= '9') { prec = prec*10 + (*s - '0'); s++; }
        }
        int lmod = 0;
        for (;;) {
            if      (*s == 'h' && lmod == 0) { lmod = 2; s++; if (*s == 'h') { lmod = 1; s++; } }
            else if (*s == 'l' && lmod == 0) { lmod = 3; s++; if (*s == 'l') { lmod = 4; s++; } }
            else if (*s == 'z') { lmod = 5; s++; }
            else break;
        }
        char conv = *s;
        char numbuf[AKARI_PRINTF_BUF];
        char *out = numbuf;
        int outlen = 0;
        char prefix[4]; int plen = 0;
        switch (conv) {
            case 'd': case 'i': {
                int64_t v;
                if      (lmod == 1) v = (signed char)va_arg(ap, int);
                else if (lmod == 2) v = (short)va_arg(ap, int);
                else if (lmod == 4) v = va_arg(ap, long long);
                else if (lmod == 5) v = (int64_t)(ptrdiff_t)va_arg(ap, ptrdiff_t);
                else                v = (lmod == 3) ? (long)va_arg(ap, long) : (int)va_arg(ap, int);
                int off = _akari_format_signed(v, numbuf, sizeof(numbuf));
                out = numbuf + off;
                if (out[0] == '-') { prefix[plen++] = '-'; out++; }
                else if (plus) { prefix[plen++] = '+'; }
                else if (sp)   { prefix[plen++] = ' '; }
                outlen = (int)strlen(out);
                if (prec >= 0 && outlen < prec) {
                    int need = prec - outlen;
                    memmove(out + need, out, (size_t)outlen + 1);
                    memset(out, '0', (size_t)need);
                    outlen += need; zero = 0;
                }
                break;
            }
            case 'u': case 'o': case 'x': case 'X': {
                uint64_t u;
                unsigned base = (conv == 'o') ? 8 : (conv == 'x' || conv == 'X') ? 16 : 10;
                if      (lmod == 1) u = (unsigned char)va_arg(ap, unsigned int);
                else if (lmod == 2) u = (unsigned short)va_arg(ap, unsigned int);
                else if (lmod == 4) u = va_arg(ap, unsigned long long);
                else if (lmod == 5) u = (uint64_t)(size_t)va_arg(ap, size_t);
                else                u = (lmod == 3) ? (unsigned long)va_arg(ap, unsigned long)
                                                    : (unsigned int)va_arg(ap, unsigned int);
                int off = _akari_format_unsigned(u, base, conv == 'X', numbuf, sizeof(numbuf));
                out = numbuf + off;
                outlen = (int)strlen(out);
                if (alt) {
                    if (conv == 'o' && (prec <= outlen) && (out[0] != '0' || outlen == 0)) {
                        prefix[plen++] = '0';
                    } else if ((conv == 'x' || conv == 'X') && u != 0) {
                        prefix[plen++] = '0';
                        prefix[plen++] = (char)conv;
                    }
                }
                if (prec >= 0 && outlen < prec) {
                    int need = prec - outlen;
                    memmove(out + need, out, (size_t)outlen + 1);
                    memset(out, '0', (size_t)need);
                    outlen += need; zero = 0;
                }
                break;
            }
            case 'c': { numbuf[0] = (char)va_arg(ap, int); out = numbuf; outlen = 1; break; }
            case 's': {
                const char *str = va_arg(ap, const char *);
                if (str == NULL) str = "(null)";
                out = (char *)str; outlen = (int)strlen(str);
                if (prec >= 0 && outlen > prec) outlen = prec;
                break;
            }
            case 'p': {
                uintptr_t u = (uintptr_t)va_arg(ap, void *);
                prefix[0] = '0'; prefix[1] = 'x'; plen = 2;
                int off = _akari_format_unsigned((uint64_t)u, 16, 0, numbuf, sizeof(numbuf));
                out = numbuf + off; outlen = (int)strlen(out);
                break;
            }
            case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': {
                double d = va_arg(ap, double);
                _akari_format_double(d, numbuf, sizeof(numbuf), prec, (char)conv);
                out = numbuf; outlen = (int)strlen(out);
                break;
            }
            case 'a': case 'A': {
                /* %a/%A hex float: fall back to %g for simplicity. */
                double d = va_arg(ap, double);
                _akari_format_double(d, numbuf, sizeof(numbuf), prec, 'g');
                out = numbuf; outlen = (int)strlen(out);
                break;
            }
            case 'n': { int *np = va_arg(ap, int *); if (np) *np = total; continue; }
            case '%': { numbuf[0] = '%'; out = numbuf; outlen = 1; break; }
            default: { numbuf[0] = *s; out = numbuf; outlen = 1; break; }
        }
        int padlen = width - outlen - plen;
        if (padlen < 0) padlen = 0;
        if (padlen > 0 && !left && !zero) {
            add = _akari_pad(ctx, putc, puts_fn, ' ', padlen);
            if (add < 0) return -1;
            total += add;
        }
        if (plen) {
            if (puts_fn) {
                add = puts_fn(ctx, prefix, plen);
            } else {
                add = 0;
                int i;
                for (i = 0; i < plen; i++) {
                    if (putc(ctx, (unsigned char)prefix[i]) < 0) return -1;
                    add++;
                }
            }
            if (add < 0) return -1;
            total += add;
        }
        if (padlen > 0 && !left && zero) {
            add = _akari_pad(ctx, putc, puts_fn, '0', padlen);
            if (add < 0) return -1;
            total += add;
        }
        if (outlen) {
            if (puts_fn) {
                add = puts_fn(ctx, out, outlen);
            } else {
                add = 0;
                int i;
                for (i = 0; i < outlen; i++) {
                    if (putc(ctx, (unsigned char)out[i]) < 0) return -1;
                    add++;
                }
            }
            if (add < 0) return -1;
            total += add;
        }
        if (padlen > 0 && left) {
            add = _akari_pad(ctx, putc, puts_fn, ' ', padlen);
            if (add < 0) return -1;
            total += add;
        }
    }
    return total;
}
