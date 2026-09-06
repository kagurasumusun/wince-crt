/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * str.c -- narrow string functions.
 *
 * Implementation in standard C, based solely on ISO C specifications.
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++) != '\0') {}
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (n && (*d = *src)) { d++; src++; n--; }
    while (n--) { *d++ = '\0'; }
    return dest;
}

char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0') {}
    return dest;
}

char *strncat(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (*d) d++;
    while (n-- && (*d = *src)) { d++; src++; }
    *d = '\0';
    return dest;
}

int strcmp(const char *a, const char *b)
{
    while (*a == *b && *a) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a == *b && *a) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

int strcasecmp(const char *a, const char *b)
{
    int ca, cb;
    do {
        ca = (unsigned char)*a; cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a'-'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a'-'A';
        a++; b++;
    } while (ca == cb && ca);
    return ca - cb;
}

int strncasecmp(const char *a, const char *b, size_t n)
{
    int ca, cb;
    while (n--) {
        ca = (unsigned char)*a; cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a'-'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a'-'A';
        if (ca != cb || ca == 0) return ca - cb;
        a++; b++;
    }
    return 0;
}

char *strchr(const char *s, int c)
{
    unsigned char ch = (unsigned char)c;
    for (;;) {
        if ((unsigned char)*s == ch) return (char *)s;
        if (*s == '\0') return NULL;
        s++;
    }
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    unsigned char ch = (unsigned char)c;
    do {
        if ((unsigned char)*s == ch) last = s;
    } while (*s++);
    return (char *)last;
}

size_t strnlen(const char *s, size_t maxlen)
{
    size_t n = 0;
    while (n < maxlen && s[n]) n++;
    return n;
}

char *stpcpy(char *d, const char *s)
{
    while ((*d++ = *s++)) {}
    return d - 1;
}

char *strndup(const char *s, size_t n)
{
    size_t sl = strnlen(s, n);
    char *r = (char *)malloc(sl + 1);
    if (!r) return NULL;
    memcpy(r, s, sl);
    r[sl] = '\0';
    return r;
}

size_t strspn(const char *s, const char *accept)
{
    size_t n = 0;
    const char *a;
    while (*s) {
        for (a = accept; *a; a++) { if (*a == *s) break; }
        if (!*a) return n;
        s++; n++;
    }
    return n;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t n = 0;
    const char *r;
    while (*s) {
        for (r = reject; *r; r++) { if (*r == *s) return n; }
        s++; n++;
    }
    return n;
}

char *strpbrk(const char *s, const char *accept)
{
    const char *a;
    while (*s) {
        for (a = accept; *a; a++) { if (*a == *s) return (char *)s; }
        s++;
    }
    return NULL;
}

char *strstr(const char *haystack, const char *needle)
{
    size_t nlen = strlen(needle);
    if (nlen == 0) return (char *)haystack;
    while (*haystack) {
        if (*haystack == *needle &&
            strncmp(haystack, needle, nlen) == 0) return (char *)haystack;
        haystack++;
    }
    return NULL;
}

static char *_strtok_save;
char *strtok(char *str, const char *delim)
{
    return strtok_r(str, delim, &_strtok_save);
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
    char *s;
    if (str == NULL) str = *saveptr;
    if (str == NULL) return NULL;
    /* skip leading delimiters */
    s = str + strspn(str, delim);
    if (*s == '\0') { *saveptr = s; return NULL; }
    /* find end of token */
    {
        size_t n = strcspn(s, delim);
        if (s[n] == '\0') {
            *saveptr = s + n;
        } else {
            s[n] = '\0';
            *saveptr = s + n + 1;
        }
    }
    return s;
}

char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *r = (char *)malloc(n);
    if (!r) return NULL;
    memcpy(r, s, n);
    return r;
}

/* MS-style underscore prefix aliases used by some CE SDK code. */
char *_strdup(const char *s) { return strdup(s); }

size_t strlcpy(char *dest, const char *src, size_t size)
{
    size_t slen = strlen(src);
    if (size) {
        size_t copylen = (slen >= size) ? size - 1 : slen;
        memcpy(dest, src, copylen);
        dest[copylen] = '\0';
    }
    return slen;
}

size_t strlcat(char *dest, const char *src, size_t size)
{
    size_t dlen = strlen(dest);
    size_t slen = strlen(src);
    if (size <= dlen) return size + slen;
    size_t copylen = size - dlen - 1;
    if (copylen > slen) copylen = slen;
    memcpy(dest + dlen, src, copylen);
    dest[dlen + copylen] = '\0';
    return dlen + slen;
}

static const char *_errstr[] = {
    [0]            = "No error",
    [EPERM]        = "Operation not permitted",
    [ENOENT]       = "No such file or directory",
    [ESRCH]        = "No such process",
    [EINTR]        = "Interrupted system call",
    [EIO]          = "I/O error",
    [ENXIO]        = "No such device or address",
    [E2BIG]        = "Argument list too long",
    [ENOEXEC]      = "Exec format error",
    [EBADF]        = "Bad file descriptor",
    [ECHILD]       = "No child processes",
    [EAGAIN]       = "Resource temporarily unavailable",
    [ENOMEM]       = "Not enough memory",
    [EACCES]       = "Permission denied",
    [EFAULT]       = "Bad address",
    [EBUSY]        = "Device or resource busy",
    [EEXIST]       = "File exists",
    [EXDEV]        = "Cross-device link",
    [ENODEV]       = "No such device",
    [ENOTDIR]      = "Not a directory",
    [EISDIR]       = "Is a directory",
    [EINVAL]       = "Invalid argument",
    [ENFILE]       = "Too many open files in system",
    [EMFILE]       = "Too many open files",
    [ENOTTY]       = "Inappropriate ioctl for device",
    [EFBIG]        = "File too large",
    [ENOSPC]       = "No space left on device",
    [ESPIPE]       = "Illegal seek",
    [EROFS]        = "Read-only file system",
    [EPIPE]        = "Broken pipe",
    [EDOM]         = "Math argument out of domain",
    [ERANGE]       = "Math result not representable",
    [EDEADLK]      = "Resource deadlock avoided",
    [ENAMETOOLONG] = "File name too long",
    [ENOLCK]       = "No locks available",
    [ENOSYS]       = "Function not implemented",
    [ENOTEMPTY]    = "Directory not empty",
    [EILSEQ]       = "Illegal byte sequence",
};

char *strerror(int errnum)
{
    if (errnum >= 0 && (size_t)errnum < sizeof(_errstr)/sizeof(_errstr[0])
        && _errstr[errnum] != NULL) {
        return (char*)_errstr[errnum];
    }
    return "Unknown error";
}

int strcoll(const char *a, const char *b) { return strcmp(a, b); }
size_t strxfrm(char *dest, const char *src, size_t n)
{
    size_t slen = strlen(src);
    if (n) {
        size_t cp = (slen >= n) ? n - 1 : slen;
        memcpy(dest, src, cp); dest[cp] = '\0';
    }
    return slen;
}

char *strsep(char **stringp, const char *delim)
{
    char *s, *tok;
    if (!stringp || !*stringp) return NULL;
    s = *stringp;
    tok = s;
    {
        size_t n = strcspn(s, delim);
        if (s[n] == '\0') *stringp = NULL;
        else { s[n] = '\0'; *stringp = s + n + 1; }
    }
    return tok;
}

char *strnstr(const char *h, const char *n, size_t len)
{
    size_t nlen = strlen(n);
    if (nlen == 0) return (char *)h;
    if (nlen > len) return NULL;
    for (size_t i = 0; i + nlen <= len; i++) {
        if (h[i] == n[0] && !strncmp(h + i, n, nlen)) return (char *)(h + i);
        if (h[i] == '\0') break;
    }
    return NULL;
}

/* BSD bstring aliases */
int bcmp(const void *a, const void *b, size_t n) { return memcmp(a, b, n); }
void bcopy(const void *src, void *dst, size_t n) { memmove(dst, src, n); }
void bzero(void *s, size_t n) { memset(s, 0, n); }
