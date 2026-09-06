/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * chkstk_sh.S -- minimal __chkstk / __alloca for SuperH WinCE
 * (secondary arch).
 */
    .text
    .global __chkstk
    .global _chkstk
    .global __alloca
    .type __chkstk, @function
__chkstk:
_chkstk:
    rts
    nop
    .size __chkstk, .-__chkstk

    .type __alloca, @function
__alloca:
    sub r15, r4
    mov r15, r0
    rts
    nop
    .size __alloca, .-__alloca
