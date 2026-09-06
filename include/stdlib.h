/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_STDLIB_H_
#define _AKARI_STDLIB_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*__akari_cmp_fn_t)(const void *, const void *);
typedef void (*atexit_fn_t)(void);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767
#define MB_CUR_MAX 2

/* numeric conversion */
int           atoi(const char *s);
long          atol(const char *s);
long long     atoll(const char *s);
long          strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
long long     strtoll(const char *s, char **endptr, int base);
unsigned long long strtoull(const char *s, char **endptr, int base);
double        atof(const char *s);
double        strtod(const char *s, char **endptr);
float         strtof(const char *s, char **endptr);
double        strtold(const char *s, char **endptr);  /* long double == double on WinCE */

/* itoa family (common MS/WinCE extensions) */
char  *itoa(int v, char *s, int radix);
char  *ltoa(long v, char *s, int radix);
char  *ultoa(unsigned long v, char *s, int radix);
char  *i64toa(long long v, char *s, int radix);
char  *ui64toa(unsigned long long v, char *s, int radix);
wchar_t *_itow(int v, wchar_t *s, int radix);
wchar_t *_ltow(long v, wchar_t *s, int radix);
wchar_t *_ultow(unsigned long v, wchar_t *s, int radix);
wchar_t *_i64tow(long long v, wchar_t *s, int radix);
wchar_t *_ui64tow(unsigned long long v, wchar_t *s, int radix);
char  *_itoa(int v, char *s, int r);
char  *_ltoa(long v, char *s, int r);
char  *_ultoa(unsigned long v, char *s, int r);
char  *_i64toa(long long v, char *s, int r);
char  *_ui64toa(unsigned long long v, char *s, int r);
wchar_t *_itow(int v, wchar_t *s, int r);
wchar_t *_ltow(long v, wchar_t *s, int r);
wchar_t *_ultow(unsigned long v, wchar_t *s, int r);
wchar_t *_i64tow(long long v, wchar_t *s, int r);
wchar_t *_ui64tow(unsigned long long v, wchar_t *s, int r);

/* gcvt/ecvt/fcvt (legacy MS CRT) */
char *gcvt(double v, int digits, char *buf);
char *ecvt(double v, int digits, int *decpt, int *sign);
char *fcvt(double v, int digits, int *decpt, int *sign);

/* memory allocation */
void   *malloc(size_t size);
void   *calloc(size_t nmemb, size_t size);
void   *realloc(void *ptr, size_t size);
void    free(void *ptr);
void   *_recalloc(void *ptr, size_t count, size_t size);
void   *_expand(void *ptr, size_t size);
size_t  _msize(void *ptr);

/* environment/exit */
void    exit(int status) __attribute__((noreturn));
void    _exit(int status) __attribute__((noreturn));
void    _Exit(int status) __attribute__((noreturn));
void    abort(void) __attribute__((noreturn));
int     atexit(void (*fn)(void));
int     _onexit(void (*fn)(void));
char   *getenv(const char *name);
int     putenv(char *s);
int     system(const char *cmd);
int     _cexit(void);
int     _c_exit(void);

/* search/sort */
void    qsort(void *base, size_t nmemb, size_t size, __akari_cmp_fn_t cmp);
void   *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
                __akari_cmp_fn_t cmp);
void   *lfind(const void *key, const void *base, size_t *nmemb, size_t size,
              __akari_cmp_fn_t cmp);
void   *lsearch(const void *key, void *base, size_t *nmemb, size_t size,
                __akari_cmp_fn_t cmp);
int     hcreate(size_t n);
void    hdestroy(void);
typedef struct entry { char *key; void *data; } ENTRY;
typedef enum { FIND, ENTER } ACTION;
ENTRY  *hsearch(ENTRY item, ACTION action);

/* random */
int     rand(void);
void    srand(unsigned int seed);

/* math */
int     abs(int i);
long    labs(long i);
long long llabs(long long i);
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;
div_t   div(int num, int denom);
ldiv_t  ldiv(long num, long denom);
lldiv_t lldiv(long long num, long long denom);

/* wide/char conversions */
int     mbtowc(wchar_t *pwc, const char *s, size_t n);
int     wctomb(char *s, wchar_t wchar);
int     mblen(const char *s, size_t n);
size_t  mbstowcs(wchar_t *dst, const char *src, size_t len);
size_t  wcstombs(char *dst, const wchar_t *src, size_t len);

/* string manipulations (MS-CRT strings) */
char   *strupr(char *s);
char   *strlwr(char *s);
char   *strset(char *s, int c);
char   *strnset(char *s, int c, size_t n);
char   *strrev(char *s);
char   *_strdup(const char *s);
wchar_t *_wcsdup(const wchar_t *s);
char   *itoa(int, char*, int);

/* alloca (compiler builtin) */
#define alloca(n) __builtin_alloca(n)

/* _MAX_PATH constant */
#ifndef _MAX_PATH
#   define _MAX_PATH 260
#endif

/* extern globals */
extern int      __argc;
extern char   **__argv;
extern wchar_t **__wargv;
extern char   **_environ;

#ifdef __cplusplus
}
#endif

#endif
