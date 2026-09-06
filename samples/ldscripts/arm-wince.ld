/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * arm-wince.ld -- linker script for ARM Windows CE PE images (subsystem 9).
 * Designed for use with lld in PE/COFF mode (ld.lld -m arm_wince_pe or
 * clang -target armv7-unknown-windows-gnu -fuse-ld=lld).
 */

OUTPUT_FORMAT(pei-arm-wince-little)
OUTPUT_ARCH(arm)
ENTRY(WinMainCRTStartup)

/* Default stack/heap sizes (can be overridden by the command line). */
__stack_size__     = 65536;
__stack_commit__   = 4096;
__heap_size__      = 1048576;
__heap_commit__    = 4096;

SECTIONS
{
    . = 0x00010000;

    .text : {
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
    }

    /* ARM Windows EH tables */
    .pdata : {
        KEEP(*(.pdata))
        KEEP(*(.pdata.*))
    }
    .xdata : {
        KEEP(*(.xdata))
        KEEP(*(.xdata.*))
    }

    .rdata : ALIGN(4) {
        *(.rdata)
        *(.rdata.*)
        *(.rodata)
        *(.rodata.*)
        *(.eh_frame)
        *(.gcc_except_table)

        /* C++ constructor/destructor tables */
        ___CTOR_LIST__ = .;
        LONG(-1);
        KEEP(*(.ctors))
        KEEP(*(.ctors.*))
        LONG(0);
        ___CTOR_END__ = .;
        ___DTOR_LIST__ = .;
        LONG(-1);
        KEEP(*(.dtors))
        KEEP(*(.dtors.*))
        LONG(0);
        ___DTOR_END__ = .;
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    }

    .data : ALIGN(4) {
        *(.data)
        *(.data.*)
        *(.data_cold)
        *(.data_cold.*)
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    .bss ALIGN(4) : {
        __bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end__ = .;
    }

    .idata ALIGN(4) : { *(.idata) *(.idata.*) }
    .edata ALIGN(4) : { *(.edata) *(.edata.*) }
    .rsrc  ALIGN(4) : { *(.rsrc) *(.rsrc.*) }
    .reloc ALIGN(4) : { *(.reloc) *(.reloc.*) }

    /DISCARD/ : {
        *(.comment)
        *(.note.GNU-stack)
        *(.note)
        *(.debug*)
    }
}
