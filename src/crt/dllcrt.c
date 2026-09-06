/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup: DLL entry-point glue for Windows CE
 * (CE 4, 5, 6).
 *
 * Clean-room implementation.  The PE loader calls _DllMainCRTStartup
 * directly.  Responsibilities:
 *
 *   DLL_PROCESS_ATTACH:
 *     - Call DisableThreadLibraryCalls to suppress THREAD_ATTACH/
 *       DETACH notifications (the CRT itself maintains no per-thread
 *       state; consumers that need per-thread hooks should use PE/COFF
 *       TLS callbacks -- those are the OS loader's responsibility,
 *       they are invoked BEFORE _DllMainCRTStartup is reached, and
 *       the linker (lld) emits the .tls directory automatically from
 *       __declspec(thread) objects).
 *     - Run .init_array constructors in forward order.
 *     - Dispatch to user DllMain.
 *
 *   DLL_PROCESS_DETACH:
 *     - Dispatch to user DllMain FIRST.
 *     - Run .fini_array destructors in reverse (LIFO) order.  We do
 *       NOT call __cxa_finalize() directly -- libc's exit() calls
 *       it on process shutdown and libc++abi registers its own
 *       .fini_array or DllMain hook to finalise DLL-static C++
 *       objects on FreeLibrary; calling __cxa_finalize here would
 *       double-run destructors on process exit.
 *
 *   DLL_THREAD_ATTACH / DLL_THREAD_DETACH:
 *     - Not normally received because of DisableThreadLibraryCalls.
 *     - Forwarded to user DllMain if they do arrive.
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

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

typedef BOOL (WINAPI *DllMain_t)(HINSTANCE, DWORD, LPVOID);

/* coredll imports. */
AKARI_DLLIMPORT void DisableThreadLibraryCalls(HMODULE);

/* DLL notification reasons (stable PE/COFF numbers from winnt.h). */
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

/* ---------- Constructor / destructor tables ---------- */
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
    return TRUE;
}

/* ---------- Entry point prototype ---------- */
BOOL USED WINAPI AKARI_ENTRY("_DllMainCRTStartup")
     _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved);

BOOL USED WINAPI AKARI_ENTRY("_DllMainCRTStartup")
     _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved)
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
        r = DllMain(hDll, reason, lpvReserved);
        break;
    default:
        r = DllMain(hDll, reason, lpvReserved);
        break;
    }
    return r;
}
