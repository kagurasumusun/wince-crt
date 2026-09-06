/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * strextra.c -- extra narrow/wide string routines that complement the
 * core mem/str/wcs files: strcasecmp, strncasecmp, strlcpy, strlcat,
 * wcsnlen, wcsdup, wcscasecmp, wcsncasecmp, wmemmove, wmemset, wmemchr,
 * wcsncat, wcsncpy, wcspbrk, wcsrchr, wcsstr, wcstok, strsep,
 * _strdup/_wcsdup, _stricmp/_strnicmp/_wcsicmp/_wcsnicmp aliases.
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h>

/* Case-insensitive aliases */
int _stricmp(const char *a, const char *b)   { return strcasecmp(a,b); }
int _strnicmp(const char *a, const char *b, size_t n){ return strncasecmp(a,b,n); }
int stricmp(const char *a, const char *b)    { return strcasecmp(a,b); }
int strnicmp(const char *a, const char *b, size_t n){ return strncasecmp(a,b,n); }

/* strlwr/strupr provided in itoa.c */
char *_strlwr(char *s); char *_strupr(char *s);
char *_strlwr(char *s) { return strlwr(s); }
char *_strupr(char *s) { return strupr(s); }

/* Wide-case aliases provided in wcs.c */
int _wcsicmp(const wchar_t *a, const wchar_t *b);
int _wcsnicmp(const wchar_t *a, const wchar_t *b, size_t n);
int _wcsicmp(const wchar_t *a, const wchar_t *b)  { return wcscasecmp(a,b); }
int _wcsnicmp(const wchar_t *a, const wchar_t *b, size_t n) { return wcsncasecmp(a,b,n); }
