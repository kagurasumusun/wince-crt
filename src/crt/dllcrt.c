/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- Windows CE DLL entry point for Clang/lld-built DLLs.
 *
 * Official name, per Microsoft's "Linking to the CRT (Windows CE
 * 5.0)": _DllMainCRTStartup (with the leading underscore, on every CE
 * architecture).  Windows CE uses the plain C calling convention --
 * __cdecl semantics -- for DLL entry functions on x86 as well (per
 * the "/ENTRY (Windows CE 5.0)" documentation), so there is no
 * @12-decorated x86 variant to provide.
 *
 * The linker's default DLL-entry search (lld-link -wince) resolves
 * the unadorned "DllMainCRTStartup" spelling on non-x86 targets
 * (verified on arm-pc-wince); on 32-bit x86 lld's default search
 * uses the desktop stdcall-decorated spelling (__DllMainCRTStartup@12),
 * which Windows CE deliberately does not use -- x86 CE DLL links must
 * pass an explicit /entry:DllMainCRTStartup (lld decorates the plain
 * name with a leading underscore on x86, which the alias below
 * provides; spelling the name with a leading underscore would
 * double-decorate it and fail).  A one-line wrapper provides the
 * unadorned spelling on every CE architecture, so linking a DLL
 * without an explicit /entry works there.
 *
 * Runtime behavior follows Microsoft's "Run-time Library Behavior
 * (Windows CE 5.0)" documentation:
 *   - DLL_PROCESS_ATTACH:  constructors for global objects run first,
 *     then the user DllMain is called; __dso_handle is set to the
 *     module handle so destructor registration can be scoped to this
 *     DLL image;
 *   - DLL_PROCESS_DETACH:  the user DllMain runs first, then the
 *     termination functions (global/static destructors) -- the
 *     reverse of attach, as documented;
 *   - DLL_THREAD_ATTACH / DLL_THREAD_DETACH (and unknown future
 *     reason codes): forwarded to DllMain; the CRT performs no
 *     per-thread initialization or termination of its own.
 *
 * Scope: startup glue only (see README for the responsibility
 * table).  atexit()/__cxa_finalize processing belongs to the C
 * library / libc++abi; Akari provides __dso_handle and runs the
 * destructor list that Clang emits for windows-gnu objects
 * (__DTOR_LIST__).
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>
#include <akari/internal.h>

/* ------------------------------------------------------------------ */
/* Types / constants (BOOL is int in the Win32 data model)            */
/* ------------------------------------------------------------------ */

typedef int         BOOL_T;
typedef akari_handle HINSTANCE_T;
typedef akari_dword  DWORD_T;
typedef void        *LPVOID_T;

#define DLL_PROCESS_ATTACH 1u
#define DLL_THREAD_ATTACH  2u
#define DLL_THREAD_DETACH  3u
#define DLL_PROCESS_DETACH 0u

/* ------------------------------------------------------------------ */
/* User DllMain.  A consumer-defined DllMain overrides this weak
 * default (which just reports success) at link time.                 */
/* ------------------------------------------------------------------ */

BOOL_T DllMain(HINSTANCE_T, DWORD_T, LPVOID_T) WEAK;

BOOL_T DllMain(HINSTANCE_T hDll, DWORD_T reason, LPVOID_T reserved)
{
    (void) hDll;
    (void) reason;
    (void) reserved;
    return 1; /* TRUE */
}

/* ------------------------------------------------------------------ */
/* DLL entry point.  The canonical CE spelling is _DllMainCRTStartup;
 * "DllMainCRTStartup" is the alias lld's default DLL-entry search
 * tries first.                                                       */
/* ------------------------------------------------------------------ */

BOOL_T DllMainCRTStartup(HINSTANCE_T, DWORD_T, LPVOID_T)
    AKARI_ENTRY("_DllMainCRTStartup");
BOOL_T DllMainCRTStartupAlias(HINSTANCE_T, DWORD_T, LPVOID_T)
    AKARI_ENTRY("DllMainCRTStartup");

BOOL_T DllMainCRTStartup(HINSTANCE_T hDll, DWORD_T reason,
                         LPVOID_T reserved)
{
    BOOL_T r;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
        __dso_handle = hDll;
        akari_run_ctors();
        r = DllMain(hDll, reason, reserved);
        break;
    case DLL_PROCESS_DETACH:
        r = DllMain(hDll, reason, reserved);
        akari_run_dtors();
        break;
    default:
        r = DllMain(hDll, reason, reserved);
        break;
    }
    return r;
}

BOOL_T DllMainCRTStartupAlias(HINSTANCE_T hDll, DWORD_T reason,
                              LPVOID_T reserved)
{
    return DllMainCRTStartup(hDll, reason, reserved);
}
