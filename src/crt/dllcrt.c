/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup: DLL entry point glue for PE/COFF
 * DLLs targeting Windows CE. Attaches to DLL_PROCESS_ATTACH by
 * initialising CRT state and running C++ constructors, then calls
 * the user-supplied DllMain (weak).
 */
#include <stddef.h>
#include <akari/compiler.h>

typedef void *HANDLE;
typedef void *HMODULE;
typedef unsigned long DWORD;
typedef void *LPVOID;
typedef int BOOL;

__declspec(dllimport) void DisableThreadLibraryCalls(HMODULE);

void _akari_errno_init(void);
void _akari_atexit_init(void);

typedef BOOL (WINAPI *DllMain_t)(HMODULE, DWORD, LPVOID);

int WINAPI DllMain(HMODULE h, DWORD reason, LPVOID reserved) __attribute__((weak));
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
        _akari_errno_init();
        _akari_atexit_init();
        DisableThreadLibraryCalls(hDll);
        _run_ctors();
    }
    return DllMain(hDll, reason, lpvReserved);
}
