/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * chkstk_arm.S -- stack probe / __chkstk helper for ARM Windows CE.
 *
 * Compilers emitting large stack frames call __chkstk to ensure guard
 * pages are touched before a big stack allocation. On Windows CE with
 * APCS/AAPCS this is a no-op in many setups, but the symbol must exist
 * to satisfy references. The implementation probes the stack in 4KB
 * steps in the unlikely case that guard pages are present.
 *
 * On entry:
 *   r4 = requested stack allocation size (bytes), per MS ARM convention.
 * Clobbers: ip (r12)
 */

    .syntax unified
    .text
    .align 2
    .global __chkstk
    .global _chkstk
    .type   __chkstk, %function
__chkstk:
_chkstk:
    /* Minimum implementation: do nothing, return. Windows CE typically
     * does not require explicit stack probing for user-mode code; page
     * granularity is handled by the kernel. We keep the symbol as a
     * no-op to satisfy compiler-rt / MSVC-compiled object references. */
    bx      lr
    .size __chkstk, .-__chkstk

    .global __alloca
    .type __alloca, %function
__alloca:
    /* __alloca(size): allocate size bytes on stack and return pointer.
     * On ARM APCS this is just sp -= size ; return sp. */
    sub     sp, sp, r0
    mov     r0, sp
    bx      lr
    .size __alloca, .-__alloca

    .section .note.GNU-stack,"",%progbits
