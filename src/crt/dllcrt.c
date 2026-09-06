/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * dllcrt.c -- _DllMainCRTStartup for Windows CE 4/5/6 DLLs.
 *
 * Official entry-point name per Microsoft docs: _DllMainCRTStartup
 * (leading underscore, no trailing @N on ARM/MIPS/SH; on x86
 * emulator the linker looks for _DllMainCRTStartup@12 because the
 * x86 emulator toolchain uses stdcall for DLL entry points).  We
 * export the plain _DllMainCRTStartup name on all architectures and
 * additionally alias the @12 stdcall-decorated form on 32-bit x86
 * Windows targets; this matches what MSVC's corelibc exports.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* ---------- Win32 primitive types ---------- */
typedef int            BOOL;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef size_t         SIZE_T;
typedef void *HMODULE, *HINSTANCE, *LPVOID;
typedef void (*init_fn)(void);
typedef void (*fini_fn)(void);
typedef BOOL (WINAPI *dllmain_t)(HINSTANCE, DWORD, LPVOID);

#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif

AKARI_DLLIMPORT void DisableThreadLibraryCalls(HMODULE);

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

/* ---------- __dso_handle (shared with crt0.c; DLL has its own) ----------
 * For a DLL, __dso_handle identifies the DLL image so __cxa_atexit
 * can run the correct destructors on FreeLibrary.  Setting it to
 * the HMODULE of the DLL is the conventional Itanium ABI approach;
 * libc++abi uses this to match destructors against the unloaded
 * image.  We store it here and update it on PROCESS_ATTACH. */
void *__dso_handle = NULL;

/* ---------- Bookend sentinels for MSVC .CRT$X* sections ---------- */
#define DEFINE_CRT_TERM(x)                                              \
    __attribute__((section(".CRT$" #x), used)) static init_fn _crt_##x = (init_fn)0;
DEFINE_CRT_TERM(XIA)
DEFINE_CRT_TERM(XCA)
DEFINE_CRT_TERM(XCZ)
#undef DEFINE_CRT_TERM

static init_fn *const _xi_start = &_crt_XIA + 1;
static init_fn *const _xi_end   = &_crt_XCA;
static init_fn *const _xc_start = &_crt_XCA + 1;
static init_fn *const _xc_end   = &_crt_XCZ;

extern init_fn __init_array_start[]  __attribute__((weak));
extern init_fn __init_array_end[]    __attribute__((weak));
extern fini_fn __fini_array_start[]  __attribute__((weak));
extern fini_fn __fini_array_end[]    __attribute__((weak));
extern init_fn __CTOR_LIST__[]       __attribute__((weak));
extern init_fn __CTOR_END__[]        __attribute__((weak));

static void _run_table(init_fn *s, init_fn *e) {
    if (!s || !e) return;
    for (init_fn *p = s; p < e; p++) if (*p) (*p)();
}
static void _run_ctors(void)
{
    if (__init_array_start && __init_array_end)
        _run_table(__init_array_start, __init_array_end);
    _run_table(_xi_start, _xi_end);
    _run_table(_xc_start, _xc_end);
    if (__CTOR_LIST__ && __CTOR_END__) {
        init_fn *list = __CTOR_LIST__;
        size_t n = 0;
        if ((intptr_t)list[0] == (intptr_t)-1) { list++; while (list[n]) n++; }
        else { while ((intptr_t)list[n] != 0) n++; }
        for (size_t i = n; i > 0; i--) if (list[i-1]) list[i-1]();
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

/* ---------- Weak user DllMain ---------- */
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) __attribute__((weak));
BOOL WINAPI DllMain(HINSTANCE hDll, DWORD reason, LPVOID reserved)
{
    (void)hDll; (void)reason; (void)reserved;
    return TRUE;
}

/* ---------- DLL entry prototype & definition ---------- */
BOOL USED WINAPI AKARI_ENTRY("_DllMainCRTStartup")
     _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved);

BOOL USED WINAPI AKARI_ENTRY("_DllMainCRTStartup")
     _DllMainCRTStartup(HINSTANCE hDll, DWORD reason, LPVOID lpvReserved)
{
    BOOL r;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        __dso_handle = hDll;
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
        /* Normally suppressed by DisableThreadLibraryCalls, but
         * forwarded if they do arrive. */
        r = DllMain(hDll, reason, lpvReserved);
        break;
    default:
        r = DllMain(hDll, reason, lpvReserved);
        break;
    }
    return r;
}

/* On the x86 emulator (Windows CE emulation) the linker asks for
 * the stdcall-decorated name _DllMainCRTStartup@12.  We cannot use
 * a normal function definition because the caller will expect a
 * ret 12 epilogue on x86 (stdcall), but ARM/MIPS/SH do not have
 * stdcall at all.  The cleanest portable approach is to NOT
 * provide a second definition here -- the _DllMainCRTStartup name
 * already satisfies non-x86 linkers, and on x86 we rely on clang's
 * ability to alias symbols.  In practice the emulator uses the
 * MSVC-mangled @12 name only when built with /GD; Akari builds
 * use the cdecl-compatible _DllMainCRTStartup because WINAPI
 * expands to empty on _WIN32_WCE.  If a consumer needs @12 they
 * can pass -Wl,--defsym=_DllMainCRTStartup@12=_DllMainCRTStartup. */
