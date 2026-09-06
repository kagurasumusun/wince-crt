/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * file_local.h -- internal layout of struct _akari_FILE.
 * Shared between stdio.c and stdio_stubs.c.
 */
#ifndef _AKARI_FILE_LOCAL_H_
#define _AKARI_FILE_LOCAL_H_
#include <stdint.h>

#define AKARI_FILE_MAGIC 0x46494c45u
#define AKARI_BUFSIZ     1024

/* File flags. */
#define _AKARI_F_READ    0x0001
#define _AKARI_F_WRITE   0x0002
#define _AKARI_F_RDWR    0x0004
#define _AKARI_F_APPEND  0x0008
#define _AKARI_F_EOF     0x0010
#define _AKARI_F_ERR     0x0020
#define _AKARI_F_BIN     0x0040  /* "b" - no CRLF translation yet. */
#define _AKARI_F_ANYBUF  0x0080  /* buffer is heap-allocated */
#define _AKARI_F_STATIC  0x0100  /* stdin/stdout/stderr, never freed */
#define _AKARI_F_EOFSEEN 0x0200
#define _AKARI_F_ERRSEEN 0x0400
#define _AKARI_F_TTY     0x0800  /* output goes to debug console (CE) */

struct _akari_FILE {
    uint32_t magic;              /* always AKARI_FILE_MAGIC */
    int      fd;                 /* Win32 HANDLE cast to int, or 0/1/2 for std */
    int      flags;
    int      ungetc_ch;          /* -1 if nothing pushed back */
    unsigned char *buf;
    int      buf_size;
    int      buf_len;            /* bytes currently present in buf */
    int      buf_pos;            /* current read/write index */
    int      eof;
    int      err;
    long     offset;             /* software-tracked seek position */
};

#endif /* _AKARI_FILE_LOCAL_H_ */
