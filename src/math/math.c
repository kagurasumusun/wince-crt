/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * math.c -- software implementations of basic FP math.
 *
 * Simple reference implementations, prioritizing correctness and size
 * over IEEE-perfect rounding. Compiler builtins (fabs, sqrt) are
 * provided by <math.h> as macros.
 */
#include <math.h>
#include <stdint.h>

/* Ensure our implementations take precedence over macro builtins so
 * &fabs / &sqrt address-taken uses still work. */
#undef fabs
#undef fabsf
#undef sqrt
#undef sqrtf

/* ---- floor / ceil / fmod ---- */

static inline double _akari_floor(double x)
{
    double i = (double)(long long)x;
    if (x >= 0.0) return i;
    /* If truncation dropped a fraction, subtract 1 */
    if (i != x) return i - 1.0;
    return i;
}

static inline double _akari_ceil(double x)
{
    double i = (double)(long long)x;
    if (x <= 0.0) return i;
    if (i != x) return i + 1.0;
    return i;
}

double floor(double x) { return _akari_floor(x); }
double ceil(double x)  { return _akari_ceil(x); }

double fmod(double x, double y)
{
    if (y == 0.0) return 0.0;
    double q = (double)(long long)(x / y);
    double r = x - q * y;
    /* Adjust sign if needed */
    if ((r > 0 && x < 0) || (r < 0 && x > 0)) r += y;
    return r;
}

int isinf(double x)
{
    union { double d; uint64_t u; } u = { x };
    return ((u.u & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL);
}

int isnan(double x) { return x != x; }
int finite(double x) { return !isinf(x) && !isnan(x); }
int isfinite(double x) { return finite(x); }

double ldexp(double x, int exp)
{
    double m = 1.0, b = 2.0;
    int e = exp;
    if (e < 0) { e = -e; b = 0.5; }
    while (e) { if (e & 1) m *= b; b *= b; e >>= 1; }
    return x * m;
}

double frexp(double x, int *exp)
{
    int e = 0;
    if (x == 0.0) { *exp = 0; return 0.0; }
    double sign = 1.0;
    if (x < 0) { sign = -1.0; x = -x; }
    if (x >= 1.0) { while (x >= 1.0) { x *= 0.5; e++; } }
    else          { while (x < 0.5)  { x *= 2.0; e--; } }
    *exp = e;
    return sign * x;
}

double modf(double x, double *iptr)
{
    double i = (x < 0) ? -_akari_floor(-x) : _akari_floor(x);
    *iptr = i;
    return x - i;
}

/* ---- exp / log / pow (series approximations) ---- */

double exp(double x)
{
    double sum = 1.0, term = 1.0;
    int i;
    for (i = 1; i < 25; i++) {
        term *= x / (double)i;
        sum += term;
    }
    return sum;
}

double log(double x)
{
    if (x <= 0.0) return -1.0;
    /* reduce using frexp into [0.5, 1) */
    int e;
    double m = frexp(x, &e);
    /* log(m) = 2 * atanh((m-1)/(m+1)) */
    double t = (m - 1.0) / (m + 1.0);
    double t2 = t * t;
    double s = t;
    double term = t;
    int i;
    for (i = 1; i < 30; i++) {
        term *= t2;
        s += term / (double)(2*i + 1);
    }
    return 2.0 * s + (double)e * 0.6931471805599453;
}

double log10(double x) { return log(x) / 2.30258509299404568402; }

double pow(double x, double y)
{
    if (y == 0.0) return 1.0;
    if (x == 0.0) return 0.0;
    return exp(y * log(x));
}

/* ---- trig (small angle series) ---- */

double sin(double x)
{
    /* range-reduce x into [-pi, pi] */
    double pi = 3.141592653589793;
    while (x >  pi) x -= 2*pi;
    while (x < -pi) x += 2*pi;
    double s = 0.0, term = x, x2 = x * x;
    int i;
    for (i = 1; i < 15; i++) {
        s += term;
        term *= -x2 / (double)((2*i)*(2*i+1));
    }
    return s;
}

double cos(double x)
{
    double pi = 3.141592653589793;
    while (x >  pi) x -= 2*pi;
    while (x < -pi) x += 2*pi;
    double s = 1.0, term = 1.0, x2 = x * x;
    int i;
    for (i = 1; i < 15; i++) {
        term *= -x2 / (double)((2*i-1)*(2*i));
        s += term;
    }
    return s;
}

double tan(double x) { return sin(x)/cos(x); }

static double _akari_sqrt(double x) {
    double g = x * 0.5;
    int i;
    if (x <= 0) return 0;
    for (i = 0; i < 30; i++) g = (g + x/g) * 0.5;
    return g;
}

double asin(double x) { return atan(x / _akari_sqrt(1.0 - x*x)); }
double acos(double x) { return 1.5707963267948966 - asin(x); }

double atan(double x)
{
    /* clamp for series convergence */
    int neg = 0;
    double s, term, x2;
    if (x < 0) { neg = 1; x = -x; }
    if (x > 1.0) return neg ? -(1.5707963267948966 - atan(1.0/x))
                             :  (1.5707963267948966 - atan(1.0/x));
    s = 0.0; term = x; x2 = x*x;
    {
        int i;
        for (i = 1; i < 30; i += 2) {
            s += term/(double)i;
            term *= -x2;
        }
    }
    return neg ? -s : s;
}

double atan2(double y, double x)
{
    if (x > 0) return atan(y/x);
    if (x < 0 && y >= 0) return atan(y/x) + 3.141592653589793;
    if (x < 0 && y < 0)  return atan(y/x) - 3.141592653589793;
    if (x == 0 && y > 0) return  1.5707963267948966;
    if (x == 0 && y < 0) return -1.5707963267948966;
    return 0.0;
}

double sinh(double x) { return (exp(x) - exp(-x)) * 0.5; }
double cosh(double x) { return (exp(x) + exp(-x)) * 0.5; }
double tanh(double x) { double e2 = exp(2*x); return (e2 - 1.0)/(e2 + 1.0); }

double sqrt(double x) { return _akari_sqrt(x); }

double fabs(double x) { return x < 0 ? -x : x; }
float  fabsf(float x) { return x < 0 ? -x : x; }

double fmin(double x, double y)
{
    if (isnan(x)) return y;
    if (isnan(y)) return x;
    return x < y ? x : y;
}
double fmax(double x, double y)
{
    if (isnan(x)) return y;
    if (isnan(y)) return x;
    return x > y ? x : y;
}
float fminf(float x, float y) { return x < y ? x : y; }
float fmaxf(float x, float y) { return x > y ? x : y; }

double copysign(double x, double y)
{
    return (y < 0) ? -fabs(x) : fabs(x);
}

double round(double x)
{
    return (x >= 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
float roundf(float x) { return (float)round((double)x); }

double trunc(double x) { return (double)(long long)x; }
float truncf(float x) { return (float)(long long)x; }

long   lround(double x) { return (long)round(x); }
long long llround(double x) { return (long long)round(x); }

float powf(float x, float y) { return (float)pow((double)x, (double)y); }
float sinf(float x) { return (float)sin((double)x); }
float cosf(float x) { return (float)cos((double)x); }
float tanf(float x) { return (float)tan((double)x); }
float sqrtf(float x) { return (float)_akari_sqrt((double)x); }
float floorf(float x) { return (float)_akari_floor((double)x); }
float ceilf(float x) { return (float)_akari_ceil((double)x); }
float fmodf(float x, float y) { return (float)fmod((double)x, (double)y); }
float expf(float x) { return (float)exp((double)x); }
float logf(float x) { return (float)log((double)x); }
float log10f(float x) { return (float)log10((double)x); }
float atanf(float x) { return (float)atan((double)x); }
float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }
float asinf(float x) { return (float)asin((double)x); }
float acosf(float x) { return (float)acos((double)x); }
float sinhf(float x) { return (float)sinh((double)x); }
float coshf(float x) { return (float)cosh((double)x); }
float tanhf(float x) { return (float)tanh((double)x); }
float ldexpf(float x, int e) { return (float)ldexp((double)x, e); }
float frexpf(float x, int *e) { return (float)frexp((double)x, e); }
