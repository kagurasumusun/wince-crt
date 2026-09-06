/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * chkstk_x86.S -- __chkstk / __alloca for x86 Windows CE.
 * (Secondary arch; primary target is ARM.)
 */
    .text
    .global __chkstk
    .global _chkstk
    .global __alloca
    .type __chkstk, @function
__chkstk:
_chkstk:
    /* On x86 CE, just probe one page at a time down the stack. */
    push    %ecx
    cmp     $0x1000, %eax
    jb      1f
2:  sub     $0x1000, %esp
    test    %eax, (%esp)
    sub     $0x1000, %eax
    cmp     $0x1000, %eax
    jae     2b
1:  sub     %eax, %esp
    test    %eax, (%esp)
    mov     %esp, %eax
    pop     %ecx
    ret
    .size __chkstk, .-__chkstk

    .type __alloca, @function
__alloca:
    mov     4(%esp), %eax
    sub     %eax, %esp
    mov     %esp, %eax
    add     $3, %eax
    and     $-4, %eax
    ret
    .size __alloca, .-__alloca
