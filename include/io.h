/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_IO_H_
#define _AKARI_IO_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int _access(const char *p, int m);
int _chmod(const char *p, int m);
int _chsize(int fd, long sz);
int _close(int fd);
int _commit(int fd);
int _creat(const char *p, int m);
int _dup(int fd);
int _dup2(int a, int b);
int _eof(int fd);
long _filelength(int fd);
int _isatty(int fd);
int _locking(int fd, int mode, long n);
long _lseek(int fd, long off, int w);
char *_mktemp(char *tpl);
int _open(const char *p, int flags, ...);
int _read(int fd, void *b, unsigned n);
int _remove(const char *p);
int _rename(const char *a, const char *b);
int _rmtmp(void);
int _setmode(int fd, int mode);
int _sopen(const char *p, int flags, int sh, ...);
long _tell(int fd);
int _umask(int m);
int _unlink(const char *p);
int _write(int fd, const void *b, unsigned n);

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH   0x20

#ifdef __cplusplus
}
#endif
#endif
