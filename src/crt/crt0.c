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
 * exact PE/COFF spellings above with GNU asm labels; on 32-bit x86
 * extra underscore-prefixed aliases (the older CE x86 toolchain
 * spelling, e.g. _WinMainCRTStartup) are provided as well.
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
 *      stdio flush) runs before ExitProcess; if no libc exit() is
 *      linked, fall back to ExitProcess directly so the entry point
 *      can never return into the loader.
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

AKARI_DLLIMPORT void ExitProcess(akari_dword) NORETURN;

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
        ExitProcess((akari_dword) rc);
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
/* x86-only aliases: the older Windows CE x86 toolchains (eVC class)
 * decorate C names with a leading underscore, so images and build
 * scripts from that world spell the entries _WinMainCRTStartup,
 * _wWinMainCRTStartup, _mainACRTStartup, _mainWCRTStartup.  Provide
 * them on 32-bit x86 only (ARM/Thumb never used the decoration; the
 * plain names above are the documented CE spellings there).         */
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
