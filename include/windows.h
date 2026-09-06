/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * windows.h -- minimal Windows API umbrella header for Windows CE.
 */
#ifndef _AKARI_WINDOWS_H_
#define _AKARI_WINDOWS_H_

#include <stddef.h>
#include <stdarg.h>
#include <akari/compiler.h>
#include <akari/windef.h>
#include <akari/winnt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Winbase-level helpers */
int WINAPI lstrlenA(LPCSTR s);
int WINAPI lstrlenW(LPCWSTR s);
LPSTR WINAPI lstrcpyA(LPSTR d, LPCSTR s);
LPWSTR WINAPI lstrcpyW(LPWSTR d, LPCWSTR s);
LPSTR WINAPI lstrcatA(LPSTR d, LPCSTR s);
LPWSTR WINAPI lstrcatW(LPWSTR d, LPCWSTR s);
int WINAPI lstrcmpA(LPCSTR a, LPCSTR b);
int WINAPI lstrcmpW(LPCWSTR a, LPCWSTR b);
int WINAPI lstrcmpiA(LPCSTR a, LPCSTR b);
int WINAPI lstrcmpiW(LPCWSTR a, LPCWSTR b);

/* ANSI shims implemented in src/misc/ansishim.c (Windows CE is Unicode-only) */
int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName);
HANDLE WINAPI CreateFileA(LPCSTR fn, DWORD acc, DWORD share, LPVOID sa,
                          DWORD create, DWORD flags, HANDLE tmpl);

HANDLE WINAPI GetStdHandle(DWORD nStdHandle);

#ifdef _UNICODE
#define MessageBox      MessageBoxW
#define GetModuleHandle GetModuleHandleW
#define CreateFile      CreateFileW
#define lstrlen         lstrlenW
#define lstrcpy         lstrcpyW
#define lstrcat         lstrcatW
#define lstrcmp         lstrcmpW
#define lstrcmpi        lstrcmpiW
#else
#define MessageBox      MessageBoxA
#define GetModuleHandle GetModuleHandleA
#define CreateFile      CreateFileA
#define lstrlen         lstrlenA
#define lstrcpy         lstrcpyA
#define lstrcat         lstrcatA
#define lstrcmp         lstrcmpA
#define lstrcmpi        lstrcmpiA
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_WINDOWS_H_ */
