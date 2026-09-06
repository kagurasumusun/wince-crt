/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * setjmp_arm.S -- setjmp / longjmp for ARM (APCS-32 / AAPCS compatible).
 *
 * Per the ARM Procedure Call Standard the callee-saved registers are
 * r4-r11, sp, lr. We also save r0 to return 0 on first return.
 * jmp_buf layout (32-bit words):
 *   [0]  r4
 *   [1]  r5
 *   [2]  r6
 *   [3]  r7
 *   [4]  r8
 *   [5]  r9
 *   [6]  r10 (sl)
 *   [7]  r11 (fp)
 *   [8]  sp
 *   [9]  lr
 *   [10] signal mask flag (unused; saved for API compat)
 */

    .syntax unified
    .text
    .align 2
    .global setjmp
    .type   setjmp, %function
setjmp:
    stmia   r0!, {r4-r11, sp, lr}
    mov     r0, #0
    bx      lr
    .size setjmp, .-setjmp

    .global longjmp
    .type longjmp, %function
longjmp:
    ldmia   r0!, {r4-r11, sp, lr}
    mov     r0, r1
    cmp     r0, #0
    movne   pc, lr
    movs    r0, #1
    bx      lr
    .size longjmp, .-longjmp

    .global sigsetjmp
    .type   sigsetjmp, %function
sigsetjmp:
    /* Second argument `savesigs' is ignored (no signals on CE) */
    b       setjmp
    .size sigsetjmp, .-sigsetjmp

    .global siglongjmp
    .type siglongjmp, %function
siglongjmp:
    b       longjmp
    .size siglongjmp, .-siglongjmp

    .section .note.GNU-stack,"",%progbits
