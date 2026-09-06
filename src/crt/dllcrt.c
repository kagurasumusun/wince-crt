/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- DLL entry point DllMainCRTStartup.
 *
 * When a DLL is loaded the OS calls DllMainCRTStartup. We run global
 * constructors on DLL_PROCESS_ATTACH, forward the call to user DllMain,
 * and run destructors / atexit handlers on DLL_PROCESS_DETACH.
 */
#include <stddef.h>
#include <akari/windef.h>
#include <akari/winnt.h>
#include <akari/compiler.h>

typedef void (*init_fn)(void);
extern init_fn __init_array_start[] __attribute__((weak));
extern init_fn __init_array_end[]   __attribute__((weak));
extern init_fn __CTOR_LIST__[]      __attribute__((weak));
extern init_fn __CTOR_END__[]       __attribute__((weak));
extern init_fn __fini_array_start[] __attribute__((weak));
extern init_fn __fini_array_end[]   __attribute__((weak));
extern init_fn __DTOR_LIST__[]      __attribute__((weak));
extern init_fn __DTOR_END__[]       __attribute__((weak));

void _akari_errno_init(void);
void _akari_atexit_register(void);
void _akari_atexit_fini(void);
void _akari_stdio_init(void);

/* User-supplied DllMain (weak default always returns TRUE). */
BOOL WINAPI DllMain(HANDLE hInstDLL, DWORD fdwReason, LPVOID lpvReserved) __attribute__((weak));

#pragma weak DllMain = _akari_default_DllMain
BOOL WINAPI _akari_default_DllMain(HANDLE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void)hInstDLL; (void)fdwReason; (void)lpvReserved;
    return TRUE;
}

static void _akari_run_ctors(void)
{
    if (__init_array_start && __init_array_end) {
        size_t n = (size_t)(__init_array_end - __init_array_start);
        size_t i;
        for (i = 0; i < n; i++) if (__init_array_start[i]) __init_array_start[i]();
    }
    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        if ((intptr_t)list[0] == -1) { list++; while (list[n]) n++; }
        else while ((intptr_t)list[n] != 0) n++;
        { size_t i; for (i = n; i > 0; i--) if (list[i-1]) list[i-1](); }
    }
}

static void _akari_run_dtors(void)
{
    if (__fini_array_start && __fini_array_end) {
        size_t n = (size_t)(__fini_array_end - __fini_array_start);
        while (n--) if (__fini_array_start[n]) __fini_array_start[n]();
    }
}

BOOL WINAPI DllMainCRTStartup(HANDLE hDll, DWORD dwReason, LPVOID lpvReserved)
{
    BOOL bRet = TRUE;
    if (dwReason == DLL_PROCESS_ATTACH) {
        _akari_errno_init();
        _akari_atexit_register();
        _akari_stdio_init();
        _akari_run_ctors();
    }
    bRet = DllMain(hDll, dwReason, lpvReserved);
    if (dwReason == DLL_PROCESS_DETACH) {
        _akari_atexit_fini();
        _akari_run_dtors();
    }
    return bRet;
}
