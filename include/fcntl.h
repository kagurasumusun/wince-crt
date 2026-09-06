/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_FCNTL_H_
#define _AKARI_FCNTL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_APPEND 0x0008
#define O_CREAT  0x0100
#define O_TRUNC  0x0200
#define O_EXCL   0x0400
#define O_BINARY 0x8000
#define O_TEXT   0x4000
#define O_ACCMODE 0x0003

#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR   O_RDWR
#define _O_APPEND O_APPEND
#define _O_CREAT  O_CREAT
#define _O_TRUNC  O_TRUNC
#define _O_EXCL   O_EXCL
#define _O_BINARY O_BINARY
#define _O_TEXT   O_TEXT

int open(const char *path, int flags, ...);
int _open(const char *path, int flags, ...);
int creat(const char *path, int mode);
int _creat(const char *path, int mode);

#ifdef __cplusplus
}
#endif
#endif
