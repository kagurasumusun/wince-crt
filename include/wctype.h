/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_WCTYPE_H_
#define _AKARI_WCTYPE_H_

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long wctype_t;
typedef int wctrans_t;

int iswalnum(wint_t wc);
int iswalpha(wint_t wc);
int iswblank(wint_t wc);
int iswcntrl(wint_t wc);
int iswdigit(wint_t wc);
int iswgraph(wint_t wc);
int iswlower(wint_t wc);
int iswprint(wint_t wc);
int iswpunct(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswxdigit(wint_t wc);
wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);
int iswctype(wint_t wc, wctype_t desc);
wctype_t wctype(const char *name);
wctrans_t wctrans(const char *name);
wint_t towctrans(wint_t wc, wctrans_t desc);

/* Wide ctype classification table offsets */
#define _UPPER  0x01
#define _LOWER  0x02
#define _DIGIT  0x04
#define _SPACE  0x08
#define _PUNCT  0x10
#define _CNTRL  0x20
#define _BLANK  0x40
#define _HEX    0x80
#define _ALPHA  (_UPPER|_LOWER)
#define _ALNUM  (_ALPHA|_DIGIT)

#ifdef __cplusplus
}
#endif
#endif
