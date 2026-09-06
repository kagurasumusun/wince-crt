/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_DIRECT_H_
#define _AKARI_DIRECT_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int _chdir(const char *path);
int _mkdir(const char *path);
int _rmdir(const char *path);
char *_getcwd(char *buf, size_t sz);
char *_getdcwd(int drive, char *buf, size_t sz);
int   _getdrive(void);
unsigned long _getdrives(void);

int  mkdir(const char *path, int mode);
int  rmdir(const char *path);
char *getcwd(char *buf, size_t sz);

#ifdef __cplusplus
}
#endif
#endif
