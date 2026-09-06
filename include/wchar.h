/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * wchar.h -- wide-character support.
 * Windows CE uses UTF-16 (unsigned short) as wchar_t.
 */
#ifndef _AKARI_WCHAR_H_
#define _AKARI_WCHAR_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long wint_t;
typedef long mbstate_t;

#ifndef WEOF
#  define WEOF ((wint_t)-1)
#endif

/* Wide-string functions */
wchar_t *wcscpy(wchar_t *d, const wchar_t *s);
wchar_t *wcsncpy(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wcscat(wchar_t *d, const wchar_t *s);
wchar_t *wcsncat(wchar_t *d, const wchar_t *s, size_t n);
size_t   wcslen(const wchar_t *s);
size_t   wcsnlen(const wchar_t *s, size_t n);
int      wcscmp(const wchar_t *a, const wchar_t *b);
int      wcsncmp(const wchar_t *a, const wchar_t *b, size_t n);
int      wcscasecmp(const wchar_t *a, const wchar_t *b);
int      wcsncasecmp(const wchar_t *a, const wchar_t *b, size_t n);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept);
size_t   wcsspn(const wchar_t *s, const wchar_t *accept);
size_t   wcscspn(const wchar_t *s, const wchar_t *reject);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
wchar_t *wcsdup(const wchar_t *s);
wchar_t *wcstok(wchar_t *str, const wchar_t *delim, wchar_t **saveptr);

wchar_t *wmemcpy(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wmemmove(wchar_t *d, const wchar_t *s, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
int      wmemcmp(const wchar_t *a, const wchar_t *b, size_t n);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);

wint_t   getwchar(void);
wint_t   putwchar(wchar_t c);
wint_t   fgetwc(FILE *stream);
wint_t   fputwc(wchar_t c, FILE *stream);
wchar_t *fgetws(wchar_t *s, int n, FILE *stream);
int      fputws(const wchar_t *s, FILE *stream);

int swprintf(wchar_t *buf, size_t n, const wchar_t *fmt, ...);
int vswprintf(wchar_t *buf, size_t n, const wchar_t *fmt, va_list ap);

/* wscanf family */
int  swscanf(const wchar_t *s, const wchar_t *fmt, ...);
int  wscanf(const wchar_t *fmt, ...);
int  fwscanf(FILE *f, const wchar_t *fmt, ...);
int  vswscanf(const wchar_t *s, const wchar_t *fmt, va_list ap);
int  vwscanf(const wchar_t *fmt, va_list ap);
int  vfwscanf(FILE *f, const wchar_t *fmt, va_list ap);

double   wcstod(const wchar_t *s, wchar_t **endptr);
float    wcstof(const wchar_t *s, wchar_t **endptr);
long double wcstold(const wchar_t *s, wchar_t **endptr);
long     wcstol(const wchar_t *s, wchar_t **endptr, int base);
unsigned long wcstoul(const wchar_t *s, wchar_t **endptr, int base);
long long wcstoll(const wchar_t *s, wchar_t **endptr, int base);
unsigned long long wcstoull(const wchar_t *s, wchar_t **endptr, int base);

/* mbs <-> wcs conversions */
size_t mbstowcs(wchar_t *dest, const char *src, size_t n);
size_t wcstombs(char *dest, const wchar_t *src, size_t n);
int    mbtowc(wchar_t *pwc, const char *s, size_t n);
int    wctomb(char *s, wchar_t wc);

#ifdef __cplusplus
}
#endif

#endif
