/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * posix_io.c -- POSIX-ish file-descriptor shims for source compatibility.
 * On Windows CE these all fail with ENOSYS because CE exposes file I/O
 * only through the Win32 CreateFile/ReadFile/WriteFile API. On host we
 * forward to raw Linux syscalls in hoststubs.c for self-tests.
 */
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <akari/compiler.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifdef _DEBUG_HOSTCHECK_
extern long _akari_host_write(int fd, const void *buf, unsigned long count);
extern long _akari_host_read(int fd, void *buf, unsigned long count);
extern long _akari_host_open(const char *p, int flags, int mode);
extern long _akari_host_close(int fd);
extern long _akari_host_lseek(int fd, long off, int whence);
extern long _akari_host_unlink(const char *p);

/* Strong alias requires the target to be defined in the same TU; provide
 * forwarding shims instead so these resolve correctly at link time. */
static inline long _Hw(int fd, const void *buf, unsigned long count){ return _akari_host_write(fd,buf,count); }
#else
static inline long _Hw(int fd, const void *buf, unsigned long count){ (void)fd;(void)buf;(void)count;return -1; }
#endif

int _isatty(int fd) { (void)fd; return 0; }
int isatty(int fd) { return _isatty(fd); }

int _close(int fd) {
#ifdef _DEBUG_HOSTCHECK_
    return (int)_akari_host_close(fd);
#else
    (void)fd; return -1;
#endif
}
int close(int fd) { return _close(fd); }

int _read(int fd, void *b, unsigned n){
#ifdef _DEBUG_HOSTCHECK_
    return (int)_akari_host_read(fd,b,(unsigned long)n);
#else
    (void)fd;(void)b;(void)n;return 0;
#endif
}
int read(int fd, void *b, unsigned n){return _read(fd,b,n);}

int _write(int fd, const void *b, unsigned n){ (void)fd;(void)b;(void)n;return -1; }
int write(int fd, const void *b, unsigned n){ return (int)_Hw(fd,b,(unsigned long)n); }

long _lseek(int fd, long off, int w){
#ifdef _DEBUG_HOSTCHECK_
    return _akari_host_lseek(fd,off,w);
#else
    (void)fd;(void)off;(void)w;return -1L;
#endif
}
long lseek(int fd, long off, int w){return _lseek(fd,off,w);}

int _unlink(const char *p){
#ifdef _DEBUG_HOSTCHECK_
    return (int)_akari_host_unlink(p);
#else
    (void)p;return -1;
#endif
}
int unlink(const char *p){return _unlink(p);}

int _access(const char *p, int m){(void)p;(void)m;return -1;}
int access(const char *p, int m){return _access(p,m);}
int _chmod(const char *p, int m){(void)p;(void)m;return -1;}
int chmod(const char *p, int m){return _chmod(p,m);}
int _chdir(const char *p){(void)p;return -1;}
int chdir(const char *p){return _chdir(p);}
int _mkdir(const char *p){(void)p;return -1;}
int mkdir(const char *p, int m){(void)p;(void)m;return _mkdir(p);}
int _rmdir(const char *p){(void)p;return -1;}
int rmdir(const char *p){return _rmdir(p);}
char *_getcwd(char *b, size_t n){(void)b;(void)n;return NULL;}
char *getcwd(char *b, size_t n){return _getcwd(b,n);}
int _dup(int fd){(void)fd;return -1;}
int dup(int fd){return _dup(fd);}
int _dup2(int a, int b){(void)a;(void)b;return -1;}
int dup2(int a, int b){return _dup2(a,b);}
int _getpid(void){return 0;}
int getpid(void){return _getpid();}
unsigned sleep(unsigned s){(void)s;return 0;}

int open(const char *p, int flags, ...){
#ifdef _DEBUG_HOSTCHECK_
    return (int)_akari_host_open(p,flags,0644);
#else
    (void)p;(void)flags;return -1;
#endif
}
int _open(const char *p, int flags, ...){
#ifdef _DEBUG_HOSTCHECK_
    return (int)_akari_host_open(p,flags,0644);
#else
    (void)p;(void)flags;return -1;
#endif
}
int creat(const char *p, int m){(void)p;(void)m;return -1;}
int _creat(const char *p, int m){return creat(p,m);}
int _chsize(int fd, long sz){(void)fd;(void)sz;return -1;}
int _commit(int fd){(void)fd;return 0;}
int _eof(int fd){(void)fd;return 1;}
long _filelength(int fd){(void)fd;return -1L;}
long _tell(int fd){return _lseek(fd,0,SEEK_CUR);}
int _umask(int m){(void)m;return 0;}
int _setmode(int fd, int m){(void)fd;(void)m;return -1;}
int _sopen(const char *p, int f, int sh, ...){(void)p;(void)f;(void)sh;return -1;}
/* remove / rename are defined in stdio.c */
int rename(const char *a, const char *b);
int _rename(const char *a, const char *b){return rename(a,b);}
int _getdrive(void){return 0;}
unsigned long _getdrives(void){return 0;}
char *_getdcwd(int d, char *b, size_t n){(void)d;(void)b;(void)n;return NULL;}
