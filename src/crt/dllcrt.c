/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup: DLL entry-point glue for Windows CE.
 *
 * Clean-room implementation.  The PE loader calls _DllMainCRTStartup
 * directly (it is the DLL entry point).  Responsibilities:
 *
 *   1. On DLL_PROCESS_ATTACH:
 *        - Disable per-thread attach/detach notifications via
 *          DisableThreadLibraryCalls.  Thread attach/detach are
 *          expensive on CE and the CRT itself never needs them;
 *          consumers that genuinely require per-thread DLL
 *          notifications (TLS-aware libraries) must NOT rely on
 *          DllMain for that -- the PE/COFF TLS callback mechanism is
 *          the correct mechanism and is NOT a CRT responsibility (the
 *          linker emits the TLS directory from objects compiled with
 *          __declspec(thread); the OS invokes the callbacks before
 *          DllMain is ever reached).
 *        - Run .init_array constructors, then dispatch to user
 *          DllMain.  Constructors run before user code so that any
 *          object DllMain touches is already constructed.
 *
 *   2. On DLL_PROCESS_DETACH:
 *        - Dispatch to user DllMain FIRST, then run .fini_array
 *          destructors in REVERSE order.  Destructors thus see the
 *          same DLL state that DllMain observed (the module has not
 *          been unmapped yet, code sections are still resident).
 *        - If lpvReserved is non-NULL the process is terminating
 *          (ExitProcess path); libc's exit() has already run
 *          atexit/__cxa_finalize at that point so we still run
 *          .fini_array (it is safe and idempotent).
 *
 *   3. DLL_THREAD_ATTACH / DLL_THREAD_DETACH:
 *        - These notifications are disabled by the
 *          DisableThreadLibraryCalls call above and will not
 *          normally arrive.  We forward them if they do (e.g. a
 *          consumer that explicitly re-enables them) but do no
 *          CRT-level bookkeeping.
 *
 * TLS callbacks (the .tls directory / __tls_used array): the CRT
 * does not register any TLS callback.  When a consumer uses
 * __declspec(thread) variables or explicit TLS callbacks, the
 * linker (lld) synthesises the TLS directory and the OS calls the
 * callbacks before _DllMainCRTStartup -- this is ABI-handled below
 * the CRT layer.
 *
 * __cxa_atexit registration for DLL unload: C++ objects with static
 * storage duration defined inside a DLL register their destructors
 * with __cxa_atexit(dtor, obj, __dso_handle).  When the DLL is
 * unloaded via FreeLibrary, libc++abi is expected to call
 * __cxa_finalize(__dso_handle) for the DLL being unloaded.  Akari
 * relies on libc (or libc++abi) to do this as part of its exit()
 * path or via a DLL-notification mechanism; Akari does NOT itself
 * call __cxa_finalize because that would duplicate libc's work on
 * process exit.  The .fini_array run on DLL_PROCESS_DETACH gives
 * non-C++ cleanup a hook regardless of libc's C++ ABI support.
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

/* DLL notification reasons (stable PE/COFF ABI numbers, from winnt.h). */
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

/* ---------- Constructors / destructors ----------
 *
 * .init_array: forward order (low address -> high), matches System
 * V ABI and Clang/lld.
 * .fini_array: forward order in the table, invoked in REVERSE
 * (high -> low), matching __cxa_atexit LIFO semantics.
 */
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
    return 1;
}

/* ---------- Entry point (forward-declared for -Wmissing-prototypes) ---------- */
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
        /* Note: we do NOT call ExitProcess or abort() here.  If the
         * DLL was loaded by the EXE and the EXE is terminating,
         * libc's exit() will call ExitProcess.  If the DLL is being
         * unloaded via FreeLibrary, the caller continues execution. */
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        /* Will not normally arrive because of
         * DisableThreadLibraryCalls above; forward if they do. */
        r = DllMain(hDll, reason, lpvReserved);
        break;
    default:
        r = DllMain(hDll, reason, lpvReserved);
        break;
    }
    return r;
}
