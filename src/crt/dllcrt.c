/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup: DLL entry-point glue for Windows CE.
 *
 * Clean-room implementation.  The PE loader calls _DllMainCRTStartup
 * directly (it is the DLL entry point).  This stub:
 *
 *   1. On DLL_PROCESS_ATTACH: disables per-thread attach/detach
 *      notifications (DisableThreadLibraryCalls) -- CE calls DLL
 *      entry points for every thread create/exit by default, which
 *      is unnecessary for statically-linked C++ DSOs that only care
 *      about process lifetime -- then runs constructors from
 *      .init_array before dispatching to the user's DllMain.
 *   2. On DLL_PROCESS_DETACH: dispatches to user DllMain, then runs
 *      destructors from .fini_array.
 *   3. On DLL_THREAD_ATTACH / DLL_THREAD_DETACH: forwarded directly
 *      to the user's DllMain (no CRT-level bookkeeping needed).
 *
 * The user-supplied DllMain is weak: if the consumer does not define
 * one, the default below simply returns TRUE.
 *
 * WINAPI expands to nothing on Windows CE (cdecl convention); this
 * file is written to compile without that knowledge being duplicated
 * locally by always going through the WINAPI macro from compiler.h.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* ---------- Win32 primitive types (local, no SDK headers) ---------- */
typedef int            BOOL;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef size_t         SIZE_T;
typedef void *HMODULE, *HINSTANCE, *LPVOID;

typedef BOOL (WINAPI *DllMain_t)(HINSTANCE, DWORD, LPVOID);

/* coredll imports. */
AKARI_DLLIMPORT void DisableThreadLibraryCalls(HMODULE);

/* DLL notification reasons (from winnt.h; numeric values are part of
 * the stable PE/COFF ABI and cannot change). */
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

/* ---------- Constructors / destructors ---------- */
typedef void (*init_fn)(void);
typedef void (*fini_fn)(void);
extern init_fn __init_array_start[]   __attribute__((weak));
extern init_fn __init_array_end[]     __attribute__((weak));
extern fini_fn __fini_array_start[]   __attribute__((weak));
extern fini_fn __fini_array_end[]     __attribute__((weak));

static void _run_ctors(void)
{
    if (__init_array_start && __init_array_end) {
        size_t n = (size_t)(__init_array_end - __init_array_start);
        for (size_t i = 0; i < n; i++)
            if (__init_array_start[i]) __init_array_start[i]();
    }
}

static void _run_dtors(void)
{
    if (__fini_array_start && __fini_array_end) {
        /* Destructors are emitted in forward order and invoked in reverse. */
        size_t n = (size_t)(__fini_array_end - __fini_array_start);
        for (size_t i = n; i > 0; i--)
            if (__fini_array_start[i-1]) __fini_array_start[i-1]();
    }
}

/* ---------- User DllMain (weak; default returns TRUE) ---------- */
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) __attribute__((weak));
BOOL WINAPI DllMain(HINSTANCE hDll, DWORD reason, LPVOID reserved)
{
    (void)hDll; (void)reason; (void)reserved;
    return 1;
}

/* ---------- Entry point called by the PE loader ----------
 *
 * Forward-declared to satisfy -Wmissing-prototypes; the PE loader
 * enters at the unmangled name _DllMainCRTStartup.
 */
BOOL USED WINAPI _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved);

BOOL USED WINAPI _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved)
{
    BOOL r;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        _run_ctors();
        r = DllMain(hDll, reason, lpvReserved);
        break;
    case DLL_PROCESS_DETACH:
        r = DllMain(hDll, reason, lpvReserved);
        _run_dtors();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        /* We called DisableThreadLibraryCalls, so these notifications
         * will not normally be received; forward them just in case. */
        r = DllMain(hDll, reason, lpvReserved);
        break;
    default:
        r = DllMain(hDll, reason, lpvReserved);
        break;
    }
    return r;
}
