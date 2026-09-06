/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * ansishim.c -- ANSI (A-suffix) shims for Windows CE, which is Unicode-only.
 *
 * Under _DEBUG_HOSTCHECK_ the W-API backends are stubbed so host
 * self-tests link without pulling in coredll symbols.
 */

#ifdef _DEBUG_HOSTCHECK_
/*
 * Define the Win32 types we need ourselves and prevent the real
 * windef.h (pulled via stdlib.h) from clobbering our empty WINAPI
 * definition below.
 */
#  ifndef WINAPI
#    define WINAPI
#  endif
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <akari/compiler.h>

#ifndef _DEBUG_HOSTCHECK_
#  include <akari/windef.h>
#  include <akari/winnt.h>
#else
#  ifndef MAX_PATH
#    define MAX_PATH 260
#  endif
   typedef unsigned short WCHAR;
   typedef const char    *LPCSTR;
   typedef const WCHAR   *LPCWSTR;
   typedef char          *LPSTR;
   typedef WCHAR         *LPWSTR;
   typedef void          *HWND;
   typedef void          *HMODULE;
   typedef void          *HANDLE;
   typedef void          *LPVOID;
   typedef unsigned int   UINT;
   typedef unsigned long  DWORD;
#endif

int WINAPI lstrlenA(LPCSTR s) { if (!s) return 0; return (int)strlen(s); }
int WINAPI lstrlenW(LPCWSTR s) { if (!s) return 0; return (int)wcslen(s); }

LPSTR WINAPI lstrcpyA(LPSTR d, LPCSTR s) { return strcpy(d, s); }
LPWSTR WINAPI lstrcpyW(LPWSTR d, LPCWSTR s) { return wcscpy(d, s); }

LPSTR WINAPI lstrcatA(LPSTR d, LPCSTR s) { return strcat(d, s); }
LPWSTR WINAPI lstrcatW(LPWSTR d, LPCWSTR s) { return wcscat(d, s); }

int WINAPI lstrcmpA(LPCSTR a, LPCSTR b) { return strcmp(a, b); }
int WINAPI lstrcmpW(LPCWSTR a, LPCWSTR b) { return wcscmp(a, b); }
int WINAPI lstrcmpiA(LPCSTR a, LPCSTR b) { return strcasecmp(a, b); }
int WINAPI lstrcmpiW(LPCWSTR a, LPCWSTR b) { return wcscasecmp(a, b); }

#ifndef _DEBUG_HOSTCHECK_

static int _a2w_buf(const char *s, WCHAR *buf, int cap)
{
    int n = 0;
    if (!s) { if (cap > 0) buf[0] = 0; return 0; }
    while (*s && n < cap - 1) { buf[n++] = (WCHAR)(unsigned char)*s; s++; }
    buf[n] = 0;
    return n;
}

int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    WCHAR txt[512], cap[256];
    _a2w_buf(lpText, txt, 512);
    _a2w_buf(lpCaption, cap, 256);
    return MessageBoxW(hWnd, txt, cap, uType);
}

HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName)
{
    if (lpModuleName == NULL) return GetModuleHandleW(NULL);
    WCHAR buf[260];
    _a2w_buf(lpModuleName, buf, 260);
    return GetModuleHandleW(buf);
}

HANDLE WINAPI CreateFileA(LPCSTR fn, DWORD acc, DWORD share, LPVOID sa,
                          DWORD create, DWORD flags, HANDLE tmpl)
{
    WCHAR buf[MAX_PATH];
    _a2w_buf(fn, buf, MAX_PATH);
    return CreateFileW(buf, acc, share, sa, create, flags, tmpl);
}

#else

int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{ (void)hWnd; (void)lpText; (void)lpCaption; (void)uType; return 0; }

HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName)
{ (void)lpModuleName; return NULL; }

HANDLE WINAPI CreateFileA(LPCSTR fn, DWORD acc, DWORD share, LPVOID sa,
                          DWORD create, DWORD flags, HANDLE tmpl)
{ (void)fn;(void)acc;(void)share;(void)sa;(void)create;(void)flags;(void)tmpl;
  return (HANDLE)-1; }

#endif
