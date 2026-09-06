/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * aeabi_idivmod.S -- __aeabi_{ui}div{mod} helpers for ARM.
 * Returns quotient in r0, remainder in r1, per ARM EABI.
 */
    .syntax unified
    .text
    .align 2
    .global __aeabi_uidivmod
    .global __aeabi_idivmod
    .global __udivsi3
    .global __umodsi3
    .global __divsi3
    .global __modsi3
    .type __aeabi_uidivmod, %function
    .type __aeabi_idivmod, %function

__udivsi3:
    /* Fallback if caller only needs quotient (r0 = n, r1 = d). */
    push    {r4, lr}
    mov     r4, r1
    bl      __aeabi_uidivmod
    pop     {r4, pc}

__umodsi3:
    push    {r4, lr}
    bl      __aeabi_uidivmod
    mov     r0, r1
    pop     {r4, pc}

__divsi3:
    push    {r4, lr}
    mov     r4, r1
    bl      __aeabi_idivmod
    pop     {r4, pc}

__modsi3:
    push    {r4, lr}
    bl      __aeabi_idivmod
    mov     r0, r1
    pop     {r4, pc}

__aeabi_uidivmod:
    /* Input: r0 = n, r1 = d. Output: r0 = q, r1 = r. */
    cmp     r1, #0
    beq     2f                 /* divide by zero: return 0 */
    mov     r2, #0             /* remainder */
    mov     r3, #1             /* bit in quotient */
    cmp     r1, #0
    /* Shift divisor left until it exceeds n. */
1:  cmp     r1, r0, lsr #1
    movls   r1, r1, lsl #1
    movls   r3, r3, lsl #1
    bls     1b
    mov     r0, #0             /* quotient */
2:  cmp     r2, r1
    subcs   r2, r2, r1
    addcs   r0, r0, r3
    movs    r3, r3, lsr #1
    movcc   r1, r1, lsr #1
    bcs     2b
    mov     r1, r2
    bx      lr

__aeabi_idivmod:
    /* Save sign and divide absolute values. */
    push    {r4, lr}
    mov     r4, #0             /* sign */
    cmp     r0, #0
    bpl     1f
    rsb     r0, r0, #0
    eor     r4, r4, #1
1:  cmp     r1, #0
    bpl     2f
    rsb     r1, r1, #0
    eor     r4, r4, #1
2:  bl      __aeabi_uidivmod
    /* Quotient sign in r4 bit 0. Remainder sign matches dividend (r4 bit 1 not used). */
    tst     r4, #1
    rsbne   r0, r0, #0
    pop     {r4, pc}

    .section .note.GNU-stack,"",%progbits
