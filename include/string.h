/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * string.h -- string & memory routines.
 */
#ifndef _AKARI_STRING_H_
#define _AKARI_STRING_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* memory */
void   *memcpy(void *dest, const void *src, size_t n);
void   *memmove(void *dest, const void *src, size_t n);
void   *memset(void *s, int c, size_t n);
int     memcmp(const void *s1, const void *s2, size_t n);
void   *memchr(const void *s, int c, size_t n);
void   *memmem(const void *h, size_t hlen, const void *n, size_t nlen);

/* ansi strings */
size_t  strlen(const char *s);
size_t  strnlen(const char *s, size_t maxlen);
char   *strcpy(char *dest, const char *src);
char   *stpcpy(char *dest, const char *src);
char   *strncpy(char *dest, const char *src, size_t n);
char   *strcat(char *dest, const char *src);
char   *strncat(char *dest, const char *src, size_t n);
int     strcmp(const char *s1, const char *s2);
int     strncmp(const char *s1, const char *s2, size_t n);
int     strcasecmp(const char *s1, const char *s2);
int     strncasecmp(const char *s1, const char *s2, size_t n);
char   *strchr(const char *s, int c);
char   *strrchr(const char *s, int c);
size_t  strspn(const char *s, const char *accept);
size_t  strcspn(const char *s, const char *reject);
char   *strpbrk(const char *s, const char *accept);
char   *strstr(const char *haystack, const char *needle);
char   *strtok(char *str, const char *delim);
char   *strdup(const char *s);
char   *strndup(const char *s, size_t n);
char   *strerror(int errnum);
char   *strtok_r(char *str, const char *delim, char **saveptr);
size_t  strlcpy(char *dest, const char *src, size_t size);
size_t  strlcat(char *dest, const char *src, size_t size);
int     strcoll(const char *a, const char *b);
size_t  strxfrm(char *dest, const char *src, size_t n);
char   *strsep(char **stringp, const char *delim);
char   *strnstr(const char *h, const char *n, size_t len);

/* bstring (BSD) aliases */
int     bcmp(const void *a, const void *b, size_t n);
void    bcopy(const void *src, void *dst, size_t n);
void    bzero(void *s, size_t n);

/* wide strings */
size_t  wcslen(const wchar_t *s);
wchar_t *wcscpy(wchar_t *dest, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wcscat(wchar_t *dest, const wchar_t *src);
wchar_t *wcsncat(wchar_t *dest, const wchar_t *src, size_t n);
int     wcscmp(const wchar_t *s1, const wchar_t *s2);
int     wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
int     wcscasecmp(const wchar_t *s1, const wchar_t *s2);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept);
size_t  wcsspn(const wchar_t *s, const wchar_t *accept);
size_t  wcscspn(const wchar_t *s, const wchar_t *reject);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
wchar_t *wcsdup(const wchar_t *s);
wchar_t *wcstok(wchar_t *str, const wchar_t *delim, wchar_t **saveptr);

/* coredll exports these under an underscore prefix in some CE versions */
char   *_strdup(const char *s);
wchar_t *_wcsdup(const wchar_t *s);

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_STRING_H_ */
