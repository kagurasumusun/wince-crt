/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stdio.h -- buffered I/O.  Windows CE has no console subsystem, so
 * stdin/stdout/stderr are stubbed and printf writes to OutputDebugStringW.
 */
#ifndef _AKARI_STDIO_H_
#define _AKARI_STDIO_H_

#include <stddef.h>
#include <stdarg.h>

/* Forward wchar_t/wint_t so stdio.h doesn't require wchar.h */
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#ifndef _WINT_T_DEFINED
typedef long wint_t;
#define _WINT_T_DEFINED
#define WEOF ((wint_t)-1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EOF
#   define EOF (-1)
#endif
#ifndef NULL
#   define NULL ((void*)0)
#endif

#define BUFSIZ      1024
#define FILENAME_MAX 260
#define FOPEN_MAX    20
#define TMP_MAX      26
#define L_tmpnam     32
#define P_tmpdir     "\\"

#define _IOFBF 0x0001
#define _IOLBF 0x0002
#define _IONBF 0x0004

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct _akari_FILE FILE;
typedef long fpos_t;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* formatted output */
int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int vsprintf(char *str, const char *fmt, va_list ap);
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

int putchar(int c);
int puts(const char *s);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int    fseek(FILE *stream, long offset, int whence);
long   ftell(FILE *stream);
void   rewind(FILE *stream);
int    fflush(FILE *stream);
int    fclose(FILE *stream);
int    feof(FILE *stream);
int    ferror(FILE *stream);
FILE  *fopen(const char *path, const char *mode);
FILE  *fopen_w(const wchar_t *path, const wchar_t *mode);
FILE  *fdopen(int fd, const char *mode);
int    fileno(FILE *stream);

/* formatted input */
int scanf(const char *fmt, ...);
int fscanf(FILE *stream, const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);
int vscanf(const char *fmt, va_list ap);
int vfscanf(FILE *stream, const char *fmt, va_list ap);
int vsscanf(const char *str, const char *fmt, va_list ap);

/* single-char I/O aliases */
int getc(FILE *stream);
int putc(int c, FILE *stream);
int getchar(void);
int ungetc(int c, FILE *stream);
char *gets(char *s);  /* unsafe; always returns NULL */

/* string streams (NUL streams feeding OutputDebugString by default) */
int _akari_vformat(char *buf, size_t size, const char *fmt, va_list ap);

void _akari_perror(const char *s);
void perror(const char *s);

/* Wide FILE open */
FILE *_wfopen(const wchar_t *path, const wchar_t *mode);
FILE *_wfreopen(const wchar_t *path, const wchar_t *mode, FILE *stream);
int   fwide(FILE *stream, int mode);

/* Threading (single-threaded no-ops) */
void flockfile(FILE *f);
void funlockfile(FILE *f);
int  ftrylockfile(FILE *f);
void clearerr_unlocked(FILE *f);
int  feof_unlocked(FILE *f);
int  ferror_unlocked(FILE *f);

/* Temp files */
char *tmpnam(char *s);
char *tempnam(const char *dir, const char *prefix);
FILE *tmpfile(void);

int _getmaxstdio(void);
int _setmaxstdio(int n);

/* Wide stdio functions (C-locale ASCII only) */
wint_t fgetwc(FILE *f);
wint_t fputwc(wchar_t c, FILE *f);
wchar_t *fgetws(wchar_t *s, int n, FILE *f);
int   fputws(const wchar_t *s, FILE *f);
wint_t getwchar(void);
wint_t putwchar(wchar_t c);
wint_t ungetwc(wint_t c, FILE *f);

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_STDIO_H_ */
