/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * math.h -- floating point math.
 * Many math functions are either compiler builtins or provided via
 * compiler-rt. This header provides declarations and a few trivial
 * wrappers / defines.
 */
#ifndef _AKARI_MATH_H_
#define _AKARI_MATH_H_

#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUGE_VAL    __builtin_huge_val()
#define INFINITY    __builtin_inf()
#define NAN         __builtin_nan("")

#define M_E         2.7182818284590452354
#define M_LOG2E     1.4426950408889634074
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440

/* Use compiler builtins where possible */
#define fabs(x)     __builtin_fabs(x)
#define fabsf(x)    __builtin_fabsf(x)
#define sqrt(x)     __builtin_sqrt(x)
#define sqrtf(x)    __builtin_sqrtf(x)

double  floor(double x);
double  ceil(double x);
double  fmod(double x, double y);
double  pow(double x, double y);
double  exp(double x);
double  log(double x);
double  log10(double x);
double  sin(double x);
double  cos(double x);
double  tan(double x);
double  asin(double x);
double  acos(double x);
double  atan(double x);
double  atan2(double y, double x);
double  sinh(double x);
double  cosh(double x);
double  tanh(double x);
double  ldexp(double x, int exp);
double  frexp(double x, int *exp);
double  modf(double x, double *iptr);

double  fmin(double x, double y);
double  fmax(double x, double y);
float   fminf(float x, float y);
float   fmaxf(float x, float y);
double  copysign(double x, double y);
double  round(double x);
float   roundf(float x);
double  trunc(double x);
float   truncf(float x);
long    lround(double x);
long long llround(double x);

/* float variants */
float   floorf(float x);
float   ceilf(float x);
float   fmodf(float x, float y);
float   powf(float x, float y);
float   expf(float x);
float   logf(float x);
float   log10f(float x);
float   sinf(float x);
float   cosf(float x);
float   tanf(float x);
float   asinf(float x);
float   acosf(float x);
float   atanf(float x);
float   atan2f(float y, float x);
float   sinhf(float x);
float   coshf(float x);
float   tanhf(float x);
float   ldexpf(float x, int exp);
float   frexpf(float x, int *exp);

int     isinf(double x);
int     isnan(double x);
int     finite(double x);
int     isfinite(double x);

/* MSVCRT compatibility helpers used internally by printf. */
long    _ftol(double x);
double  _fptostr(double x);

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_MATH_H_ */
