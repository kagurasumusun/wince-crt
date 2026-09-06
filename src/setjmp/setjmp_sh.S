/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
    .text
    .global setjmp
    .global longjmp
    .type setjmp, @function
setjmp:
    rts
    mov #0, r0
    .size setjmp, .-setjmp

    .type longjmp, @function
longjmp:
    mov r5, r0
    tst r0, r0
    bf 1f
    mov #1, r0
1:  rts
    nop
    .size longjmp, .-longjmp
