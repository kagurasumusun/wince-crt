/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * hoststubs.c -- Linux syscall wrappers used by host-side self-tests
 *                (stdio, files, process exit). Not used on WinCE.
 */
#include <akari/compiler.h>

#ifdef _DEBUG_HOSTCHECK_

/* SYS_read=0, SYS_write=1, SYS_open=2, SYS_close=3, SYS_lseek=8, SYS_unlink=87, SYS_exit=60 */
#define _AKARI_SYS_read   0
#define _AKARI_SYS_write  1
#define _AKARI_SYS_open   2
#define _AKARI_SYS_close  3
#define _AKARI_SYS_lseek  8
#define _AKARI_SYS_unlink 87
#define _AKARI_SYS_exit   60

#define _AKARI_SYSCALL3(nr,a1,a2,a3) ({ long _r; __asm__ __volatile__( \
    "movq %1,%%rax\nmovq %2,%%rdi\nmovq %3,%%rsi\nmovq %4,%%rdx\nsyscall\nmovq %%rax,%0" \
    :"=r"(_r):"r"((long)(nr)),"r"((long)(a1)),"r"((long)(a2)),"r"((long)(a3)) \
    :"rax","rdi","rsi","rdx","rcx","r11","memory"); _r; })
#define _AKARI_SYSCALL2(nr,a1,a2) _AKARI_SYSCALL3(nr,a1,a2,0)
#define _AKARI_SYSCALL1(nr,a1)    _AKARI_SYSCALL3(nr,a1,0,0)

static inline long _fix(long r){ if((unsigned long)r >= (unsigned long)-4095) return -1; return r; }

long _akari_host_write(int fd, const void *buf, unsigned long count) { return _fix(_AKARI_SYSCALL3(_AKARI_SYS_write,fd,buf,count)); }
long _akari_host_read(int fd, void *buf, unsigned long count)       { return _fix(_AKARI_SYSCALL3(_AKARI_SYS_read,fd,buf,count)); }
long _akari_host_open(const char *p, int flags, int mode)           { return _fix(_AKARI_SYSCALL3(_AKARI_SYS_open,p,flags,mode)); }
long _akari_host_close(int fd)                                      { return _fix(_AKARI_SYSCALL1(_AKARI_SYS_close,fd)); }
long _akari_host_lseek(int fd, long off, int whence)                { return _fix(_AKARI_SYSCALL3(_AKARI_SYS_lseek,fd,off,whence)); }
long _akari_host_unlink(const char *p)                              { return _fix(_AKARI_SYSCALL1(_AKARI_SYS_unlink,p)); }

#endif /* _DEBUG_HOSTCHECK_ */
