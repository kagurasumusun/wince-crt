/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stdint.h -- ISO C99 fixed-width integer types.
 */
#ifndef _AKARI_STDINT_H_
#define _AKARI_STDINT_H_

#include <limits.h>

typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
typedef long long           int64_t;
typedef unsigned long long  uint64_t;

typedef int8_t              int_least8_t;
typedef uint8_t             uint_least8_t;
typedef int16_t             int_least16_t;
typedef uint16_t            uint_least16_t;
typedef int32_t             int_least32_t;
typedef uint32_t            uint_least32_t;
typedef int64_t             int_least64_t;
typedef uint64_t            uint_least64_t;

typedef int8_t              int_fast8_t;
typedef uint8_t             uint_fast8_t;
typedef int32_t             int_fast16_t;
typedef uint32_t            uint_fast16_t;
typedef int32_t             int_fast32_t;
typedef uint32_t            uint_fast32_t;
typedef int64_t             int_fast64_t;
typedef uint64_t            uint_fast64_t;

typedef __INTPTR_TYPE__     intptr_t;
typedef __UINTPTR_TYPE__    uintptr_t;

typedef int64_t             intmax_t;
typedef uint64_t            uintmax_t;

#define INT8_MIN            (-128)
#define INT8_MAX            127
#define UINT8_MAX           255U
#define INT16_MIN           (-32768)
#define INT16_MAX           32767
#define UINT16_MAX          65535U
#define INT32_MIN           (-2147483647-1)
#define INT32_MAX           2147483647
#define UINT32_MAX          4294967295U
#define INT64_MIN           (-9223372036854775807LL-1)
#define INT64_MAX           9223372036854775807LL
#define UINT64_MAX          18446744073709551615ULL

#define INTPTR_MIN          ((intptr_t) -(((uintptr_t)1 << (sizeof(intptr_t)*8 -1))))
#define INTPTR_MAX          ((intptr_t)(((uintptr_t)1 << (sizeof(intptr_t)*8 -1)) - 1))
#define UINTPTR_MAX         ((uintptr_t)-1)

#define INTMAX_MIN          INT64_MIN
#define INTMAX_MAX          INT64_MAX
#define UINTMAX_MAX         UINT64_MAX

#define PTRDIFF_MIN         INTPTR_MIN
#define PTRDIFF_MAX         INTPTR_MAX
#define SIZE_MAX            UINTPTR_MAX
#define SIG_ATOMIC_MIN      INT32_MIN
#define SIG_ATOMIC_MAX      INT32_MAX

#define WCHAR_MIN           0U
#define WCHAR_MAX           65535U

#define INT8_C(x)   (x)
#define UINT8_C(x)  (x##U)
#define INT16_C(x)  (x)
#define UINT16_C(x) (x##U)
#define INT32_C(x)  (x##L)
#define UINT32_C(x) (x##UL)
#define INT64_C(x)  (x##LL)
#define UINT64_C(x) (x##ULL)
#define INTMAX_C(x) (x##LL)
#define UINTMAX_C(x)(x##ULL)

#endif /* _AKARI_STDINT_H_ */
