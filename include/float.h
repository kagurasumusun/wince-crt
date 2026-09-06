/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * float.h -- IEEE 754 floating point characteristics.
 */
#ifndef _AKARI_FLOAT_H_
#define _AKARI_FLOAT_H_

#define FLT_RADIX       2

#define FLT_MANT_DIG    24
#define DBL_MANT_DIG    53
#define LDBL_MANT_DIG   53

#define FLT_DIG         6
#define DBL_DIG         15
#define LDBL_DIG        15

#define FLT_MIN_EXP     (-125)
#define DBL_MIN_EXP     (-1021)
#define LDBL_MIN_EXP    (-1021)

#define FLT_MIN_10_EXP  (-37)
#define DBL_MIN_10_EXP  (-307)
#define LDBL_MIN_10_EXP (-307)

#define FLT_MAX_EXP     128
#define DBL_MAX_EXP     1024
#define LDBL_MAX_EXP    1024

#define FLT_MAX_10_EXP  38
#define DBL_MAX_10_EXP  308
#define LDBL_MAX_10_EXP 308

#define FLT_MAX         3.40282346638528859811704183484516925e+38F
#define DBL_MAX         1.79769313486231570814527423731704357e+308
#define LDBL_MAX        DBL_MAX

#define FLT_EPSILON     1.19209289550781250000000000000000000e-07F
#define DBL_EPSILON     2.22044604925031308084726333618164062e-16
#define LDBL_EPSILON    DBL_EPSILON

#define FLT_MIN         1.17549435082228750797e-38F
#define DBL_MIN         2.22507385850720138309e-308
#define LDBL_MIN        DBL_MIN

#define FLT_ROUNDS      1
#define FLT_EVAL_METHOD 0
#define DECIMAL_DIG     17

#endif /* _AKARI_FLOAT_H_ */
