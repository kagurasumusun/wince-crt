/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * unistd.h -- POSIX-like I/O shims. Windows CE does not have the POSIX
 * file model; these are provided for source compatibility and always
 * fail with ENOSYS when built for the device.
 */
#ifndef _AKARI_UNISTD_H_
#define _AKARI_UNISTD_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int     close(int fd);
int     _close(int fd);
int     read(int fd, void *buf, unsigned int cnt);
int     _read(int fd, void *buf, unsigned int cnt);
int     write(int fd, const void *buf, unsigned int cnt);
int     _write(int fd, const void *buf, unsigned int cnt);
long    lseek(int fd, long offset, int whence);
long    _lseek(int fd, long offset, int whence);
int     unlink(const char *path);
int     _unlink(const char *path);
int     isatty(int fd);
int     _isatty(int fd);
int     dup(int fd);
int     _dup(int fd);
int     dup2(int fd1, int fd2);
int     _dup2(int fd1, int fd2);
int     access(const char *path, int mode);
int     _access(const char *path, int mode);
int     chdir(const char *path);
int     _chdir(const char *path);
int     rmdir(const char *path);
int     mkdir(const char *path, int mode);
int     getpid(void);
unsigned sleep(unsigned sec);
void    _exit(int status) __attribute__((noreturn));

/* access() modes */
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

/* lseek() whence */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifdef __cplusplus
}
#endif
#endif
