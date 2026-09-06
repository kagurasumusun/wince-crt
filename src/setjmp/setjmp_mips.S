/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
    .text
    .globl setjmp
    .globl longjmp
    .ent setjmp
setjmp:
    sw $16, 0($4)
    sw $17, 4($4)
    sw $18, 8($4)
    sw $19, 12($4)
    sw $20, 16($4)
    sw $21, 20($4)
    sw $22, 24($4)
    sw $23, 28($4)
    sw $29, 32($4)   /* sp */
    sw $30, 36($4)   /* ra */
    sw $31, 40($4)   /* fp */
    move $2, $0
    jr $31
    nop
    .end setjmp

    .ent longjmp
longjmp:
    move $2, $5
    bne $2, $0, 1f
    li $2, 1
1:  lw $16, 0($4)
    lw $17, 4($4)
    lw $18, 8($4)
    lw $19, 12($4)
    lw $20, 16($4)
    lw $21, 20($4)
    lw $22, 24($4)
    lw $23, 28($4)
    lw $29, 32($4)
    lw $30, 36($4)
    lw $31, 40($4)
    jr $30
    nop
    .end longjmp
