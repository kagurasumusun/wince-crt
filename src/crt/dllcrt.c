/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup: DLL entry-point glue.  Initialises
 * per-DLL state, disables thread-library calls, runs .init_array
 * constructors and then dispatches to the user-supplied weak DllMain.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

typedef void *HMODULE, *LPVOID;
typedef unsigned long DWORD;
typedef int BOOL;
typedef int (WINAPI *DllMain_t)(HMODULE, DWORD, LPVOID);

AKARI_DLLIMPORT void DisableThreadLibraryCalls(HMODULE);

int WINAPI DllMain(HMODULE, DWORD, LPVOID) __attribute__((weak));
int WINAPI DllMain(HMODULE h, DWORD reason, LPVOID reserved) {
    (void)h; (void)reason; (void)reserved; return 1;
}

typedef void (*init_fn)(void);
extern init_fn __init_array_start[] __attribute__((weak));
extern init_fn __init_array_end[]   __attribute__((weak));

static void _run_ctors(void) {
    if (__init_array_start && __init_array_end) {
        size_t n = (size_t)(__init_array_end - __init_array_start);
        for (size_t i = 0; i < n; i++)
            if (__init_array_start[i]) __init_array_start[i]();
    }
}

int WINAPI _DllMainCRTStartup(HMODULE hDll, DWORD reason, LPVOID lpvReserved)
{
    if (reason == 1 /* DLL_PROCESS_ATTACH */) {
        DisableThreadLibraryCalls(hDll);
        _run_ctors();
    }
    return DllMain(hDll, reason, lpvReserved);
}
