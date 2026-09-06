/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * chkstk_mips.S -- minimal __chkstk / __alloca for MIPS Windows CE
 * (secondary arch; primary target is ARM).
 */
    .text
    .globl __chkstk
    .globl _chkstk
    .globl __alloca
    .ent __chkstk
__chkstk:
_chkstk:
    jr  $31
    nop
    .end __chkstk

    .ent __alloca
__alloca:
    subu $sp, $sp, $4
    move $2, $sp
    jr   $31
    nop
    .end __alloca
