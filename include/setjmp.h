/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * setjmp.h -- non-local jumps.
 */
#ifndef _AKARI_SETJMP_H_
#define _AKARI_SETJMP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The size of jmp_buf is architecture dependent. We reserve room for 64
 * 32-bit words which is sufficient for all supported 32-bit Windows CE
 * architectures (ARM, x86, MIPS, SH).
 */
typedef uint32_t jmp_buf[64];

int  setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

/* BSD-style aliases */
typedef jmp_buf sigjmp_buf;
int  sigsetjmp(sigjmp_buf env, int savesigs);
void siglongjmp(sigjmp_buf env, int val) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_SETJMP_H_ */
