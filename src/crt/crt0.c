/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt0.c -- Windows CE EXE entry points for Clang/lld-built programs.
 *
 * Official entry-point names, per Microsoft's "Linking to the CRT
 * (Windows CE 5.0)" and "/ENTRY (Windows CE 5.0)" documentation:
 *
 *   mainACRTStartup     applications that define main()
 *   mainWCRTStartup     applications that define wmain()
 *   WinMainCRTStartup   applications that define WinMain()
 *   wWinMainCRTStartup  applications that define wWinMain()
 *   _DllMainCRTStartup  DLLs (see dllcrt.c)
 *
 * Microsoft documents these entry functions and the user functions
 * they call as __cdecl on Windows CE (no stdcall @n decoration on
 * x86).  The same names are also valid on ARM/Thumb where C names
 * carry no decoration at all.  The C identifiers are pinned to the
 * exact PE/COFF spellings above with asm labels; on 32-bit x86 extra
 * underscore-prefixed aliases (e.g. _WinMainCRTStartup) are provided
 * as well, matching the decorated spelling that an x86 C compiler
 * gives these C names (leading underscore; Microsoft's documented
 * x86 C name decoration, observable in i686 Clang output too).
 *
 * Windows CE defines no console/subsystem split for EXEs beyond the
 * /SUBSYSTEM:WINDOWSCE header: which of main/wmain/WinMain/wWinMain
 * the image defines selects the user function, and mainCRTStartup
 * (the desktop name) is deliberately NOT provided -- the Windows CE
 * main() entry is mainACRTStartup.
 *
 * Startup sequence (documented behavior + clean-room design):
 *   1. parse the command line (GetCommandLineW) into __argc/__wargv
 *      and compute _wcmdtail for WinMain's lpCmdLine;
 *   2. synthesize narrow __argv/_acmdln via WideCharToMultiByte
 *      (lossy fallback when the converter is missing from the image);
 *   3. run global constructors (.CRT$XI* C, .CRT$XC* C++, and the
 *      lld .ctors list);
 *   4. call the user entry function;
 *   5. run global destructors, then hand off to the C library's
 *      exit() so libc-owned termination (atexit/__cxa_finalize,
 *      stdio flush) runs before the process is terminated; if no libc
 *      exit() is linked, terminate the process directly
 *      (TerminateProcess) so the entry point can never return into
 *      the loader.
 *
 * Scope: this file is startup glue only -- no libc functions are
 * implemented here (see README for the responsibility table).
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>
#include <akari/internal.h>

/* ------------------------------------------------------------------ */
/* Types / constants (local; no SDK headers)                          */
/* ------------------------------------------------------------------ */

typedef akari_handle HINSTANCE_T;
typedef akari_dword  DWORD_T;

#define SW_SHOW 1

/* Process termination on Windows CE.
 *
 * coredll does NOT export ExitProcess on any CE generation (absent
 * from every CE 4/5/6 coredll import library of the toolchain
 * sysroot).  Microsoft's CE documentation does include an
 * "ExitProcess (Windows CE 5.0)" page (MSDN archive ms885217) that
 * lists "OS Versions: Windows CE 2.0 and later" and "Link Library:
 * Coredll.lib", but no CE 4/5/6 import library exports it and the CE
 * toolchain headers of the sysroot declare ExitProcess as an inline
 * TerminateProcess(GetCurrentProcess(), code) wrapper instead; the
 * doc page is treated as inaccurate on that export point.  The CE
 * termination call is TerminateProcess, which the CE documentation
 * (MSDN archive aa450927: "OS Versions: Windows CE 1.0 and later",
 * Link Library: Coredll.lib) and every CE coredll import library
 * agree on.  Its first argument is the handle of the process to
 * end; for the calling process the CE system-handle space defines a
 * fixed pseudo-handle for "the current process" (SDK kfuncs.h:
 * GetCurrentProcess() = SH_CURPROC(2) + SYS_HANDLE_BASE(64) = 66;
 * the value 66 was also observed in the compiled CE CRT objects of
 * the toolchain sysroot). */
#define AKARI_CURRENT_PROCESS ((akari_handle) (uintptr_t) 66u)

AKARI_DLLIMPORT int TerminateProcess(akari_handle, akari_dword)
    __asm__("TerminateProcess");

/* C library exit(); imported weakly: the consumer's C library (which
 * owns atexit/__cxa_finalize and the final process exit) provides the
 * strong definition.  NULL when no C library is linked. */
extern void exit(int) WEAK NORETURN;

/* ------------------------------------------------------------------ */
/* User entry points (weak).  Windows CE provides only the wide
 * command line; WinMain's lpCmdLine is LPWSTR.  hPrevInstance is
 * always NULL on Windows CE (per the CE WinMain documentation) and
 * envp is NULL (no environment block exists on Windows CE).         */
/* ------------------------------------------------------------------ */

int WINAPI WinMain(HINSTANCE_T, HINSTANCE_T, akari_wchar *, int) WEAK;
int WINAPI wWinMain(HINSTANCE_T, HINSTANCE_T, akari_wchar *, int) WEAK;
int main(int, char **, char **) WEAK;
int wmain(int, akari_wchar **, akari_wchar **) WEAK;

/* ------------------------------------------------------------------ */
/* Hand-off to the operating system                                   */
/* ------------------------------------------------------------------ */

static void os_exit(int rc) NORETURN;

static void os_exit(int rc)
{
    if (exit) {
        exit(rc);
        /* not reached when a correct libc is linked */
    }
    for (;;) {
        TerminateProcess(AKARI_CURRENT_PROCESS, (akari_dword) rc);
    }
}

/* ------------------------------------------------------------------ */
/* EXE entry functions                                                */
/* ------------------------------------------------------------------ */

void WinMainCRTStartup(void) AKARI_ENTRY("WinMainCRTStartup");
void wWinMainCRTStartup(void) AKARI_ENTRY("wWinMainCRTStartup");
void mainACRTStartup(void) AKARI_ENTRY("mainACRTStartup");
void mainWCRTStartup(void) AKARI_ENTRY("mainWCRTStartup");

/* The PE loader starts the image here; none of these return. */

void WinMainCRTStartup(void)
{
    HINSTANCE_T hinst;
    akari_wchar *tail;
    int rc;

    hinst = (HINSTANCE_T) akari_image_handle();
    akari_init_args();
    akari_run_ctors();
    tail = _wcmdtail ? _wcmdtail : (akari_wchar *) 0;

    if (WinMain) {
        rc = WinMain(hinst, (HINSTANCE_T) 0, tail, SW_SHOW);
    } else if (wWinMain) {
        rc = wWinMain(hinst, (HINSTANCE_T) 0, tail, SW_SHOW);
    } else if (main) {
        rc = main(__argc, __argv, (char **) 0);
    } else if (wmain) {
        rc = wmain(__argc, __wargv, (akari_wchar **) 0);
    } else {
        rc = 0;
    }
    akari_run_dtors();
    os_exit(rc);
}

void wWinMainCRTStartup(void)
{
    HINSTANCE_T hinst;
    akari_wchar *tail;
    int rc;

    hinst = (HINSTANCE_T) akari_image_handle();
    akari_init_args();
    akari_run_ctors();
    tail = _wcmdtail ? _wcmdtail : (akari_wchar *) 0;

    if (wWinMain) {
        rc = wWinMain(hinst, (HINSTANCE_T) 0, tail, SW_SHOW);
    } else if (WinMain) {
        rc = WinMain(hinst, (HINSTANCE_T) 0, tail, SW_SHOW);
    } else if (main) {
        rc = main(__argc, __argv, (char **) 0);
    } else if (wmain) {
        rc = wmain(__argc, __wargv, (akari_wchar **) 0);
    } else {
        rc = 0;
    }
    akari_run_dtors();
    os_exit(rc);
}

void mainACRTStartup(void)
{
    int rc;

    akari_init_args();
    akari_run_ctors();

    if (main) {
        rc = main(__argc, __argv, (char **) 0);
    } else if (wmain) {
        rc = wmain(__argc, __wargv, (akari_wchar **) 0);
    } else {
        rc = 0;
    }
    akari_run_dtors();
    os_exit(rc);
}

void mainWCRTStartup(void)
{
    int rc;

    akari_init_args();
    akari_run_ctors();

    if (wmain) {
        rc = wmain(__argc, __wargv, (akari_wchar **) 0);
    } else if (main) {
        rc = main(__argc, __argv, (char **) 0);
    } else {
        rc = 0;
    }
    akari_run_dtors();
    os_exit(rc);
}

/* ------------------------------------------------------------------ */
/* x86-only aliases: on 32-bit x86, C names carry a leading underscore
 * (Microsoft's documented x86 C name decoration; also visible in i686
 * Clang output), so link lines and build tools may spell the entries
 * _WinMainCRTStartup, _wWinMainCRTStartup, _mainACRTStartup,
 * _mainWCRTStartup.  Provide them on 32-bit x86 only; on ARM/Thumb C
 * names carry no decoration and the plain names above are the CE
 * spellings there.                                                  */
/* ------------------------------------------------------------------ */

#if AKARI_CPU_X86

void akari_alt_WinMainCRTStartup(void) AKARI_ENTRY("_WinMainCRTStartup");
void akari_alt_wWinMainCRTStartup(void) AKARI_ENTRY("_wWinMainCRTStartup");
void akari_alt_mainACRTStartup(void) AKARI_ENTRY("_mainACRTStartup");
void akari_alt_mainWCRTStartup(void) AKARI_ENTRY("_mainWCRTStartup");

void akari_alt_WinMainCRTStartup(void)
{
    WinMainCRTStartup();
}
void akari_alt_wWinMainCRTStartup(void)
{
    wWinMainCRTStartup();
}
void akari_alt_mainACRTStartup(void)
{
    mainACRTStartup();
}
void akari_alt_mainWCRTStartup(void)
{
    mainWCRTStartup();
}

#endif /* AKARI_CPU_X86 */
