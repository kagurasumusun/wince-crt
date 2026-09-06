/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * akari/windef.h -- scalar type definitions and calling conventions
 *
 * Mirrors the Win32 type conventions for Windows CE.  All types are
 * described in terms of standard C99 fixed-width types to avoid reliance
 * on <windows.h> from the Win32 SDK.
 */
#ifndef _AKARI_WINDEF_H_
#define _AKARI_WINDEF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Fundamental scalar types                                           */
/* ------------------------------------------------------------------ */

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned long       ULONG;
typedef long                LONG;
typedef short               SHORT;
typedef int                 INT;
typedef unsigned int        UINT;
typedef int                 BOOL;
typedef char                CHAR;
typedef unsigned char       UCHAR;
typedef unsigned short      WCHAR;          /* UTF-16 LE */
typedef unsigned short      USHORT;
typedef void                VOID;

typedef float               FLOAT;

typedef intptr_t            INT_PTR;
typedef uintptr_t           UINT_PTR;
typedef intptr_t            LONG_PTR;
typedef uintptr_t           ULONG_PTR;
typedef LONG_PTR            SSIZE_T;
typedef ULONG_PTR           SIZE_T;

/* Handles are just pointers to arbitrary-sized structs in memory. */
typedef void               *HANDLE;
typedef HANDLE              HWND;
typedef HANDLE              HINSTANCE;
typedef HINSTANCE           HMODULE;
typedef HANDLE              HKEY;
typedef HANDLE              HDC;
typedef HANDLE              HBITMAP;
typedef HANDLE              HMENU;
typedef HANDLE              HEVENT;
typedef HANDLE              HFILE;

typedef unsigned long       WPARAM;
typedef long                LPARAM;
typedef long                LRESULT;

typedef const char         *LPCSTR;
typedef char               *LPSTR;
typedef const WCHAR        *LPCWSTR;
typedef WCHAR              *LPWSTR;

#ifdef _UNICODE
typedef WCHAR               TCHAR;
typedef LPCWSTR             LPCTSTR;
typedef LPWSTR              LPTSTR;
#else
typedef CHAR                TCHAR;
typedef LPCSTR              LPCTSTR;
typedef LPSTR               LPTSTR;
#endif

typedef VOID               *LPVOID;
typedef const VOID         *LPCVOID;
typedef BYTE               *LPBYTE;

typedef DWORD              *LPDWORD;
typedef LONG               *PLONG;

/* ------------------------------------------------------------------ */
/*  TRUE/FALSE/NULL/size helpers                                       */
/* ------------------------------------------------------------------ */

#ifndef FALSE
#   define FALSE 0
#endif
#ifndef TRUE
#   define TRUE  1
#endif
#ifndef NULL
#   ifdef __cplusplus
#       define NULL 0
#   else
#       define NULL ((void*)0)
#   endif
#endif

#define MAX_PATH        260

/* ------------------------------------------------------------------ */
/*  Calling conventions                                                */
/* ------------------------------------------------------------------ */

/*
 * Windows CE / ARM historically uses APCS-32 (old ABI), where cdecl and
 * stdcall are identical (all arguments on stack, callee-saves r4-r11,
 * caller cleans stack).  For cross-platform consistency we still define
 * the standard decorations; on ARM they expand to nothing, while x86
 * targets will expand them to the usual __attribute__((__stdcall__)).
 */

#if defined(__arm__) || defined(__thumb__) || defined(_M_ARM) || defined(_M_ARMT)
#   define AKARI_ARCH_ARM   1
#   define __cdecl
#   define __stdcall
#   define __fastcall
#elif defined(__i386) || defined(_M_IX86)
#   define AKARI_ARCH_X86   1
#   define __cdecl      __attribute__((__cdecl__))
#   define __stdcall    __attribute__((__stdcall__))
#   define __fastcall   __attribute__((__fastcall__))
#elif defined(__mips__) || defined(_M_MRX000)
#   define AKARI_ARCH_MIPS  1
#   define __cdecl
#   define __stdcall
#   define __fastcall
#elif defined(__sh__)
#   define AKARI_ARCH_SH    1
#   define __cdecl
#   define __stdcall
#   define __fastcall
#else
#   define __cdecl
#   define __stdcall
#   define __fastcall
#endif

#ifndef WINAPI
#  define WINAPI    __stdcall
#endif
#ifndef WINAPIV
#  define WINAPIV   __cdecl
#endif
#ifndef APIENTRY
#  define APIENTRY  WINAPI
#endif
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#define CALLBACK    __stdcall

#define NEAR
#define FAR
#define CONST       const

/* FARPROC is a generic function pointer type used by GetProcAddress etc. */
typedef int (WINAPI *FARPROC)(void);

/* ------------------------------------------------------------------ */
/*  HRESULT, status codes                                              */
/* ------------------------------------------------------------------ */

typedef long HRESULT;

#define S_OK            ((HRESULT)0x00000000L)
#define S_FALSE         ((HRESULT)0x00000001L)
#define E_FAIL          ((HRESULT)0x80004005L)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000EL)
#define E_INVALIDARG    ((HRESULT)0x80070057L)
#define E_NOINTERFACE   ((HRESULT)0x80004002L)
#define E_NOTIMPL       ((HRESULT)0x80004001L)
#define E_ACCESSDENIED  ((HRESULT)0x80070005L)
#define E_HANDLE        ((HRESULT)0x80070006L)
#define E_PENDING       ((HRESULT)0x8000000AL)

#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#define HRESULT_CODE(hr)   ((hr) & 0xFFFF)
#define HRESULT_FACILITY(hr) (((hr) >> 16) & 0x1FFF)
#define HRESULT_SEVERITY(hr) (((hr) >> 31) & 0x1)
#define MAKE_HRESULT(sev,fac,code) \
        ((HRESULT)(((unsigned long)(sev)<<31)|((unsigned long)(fac)<<16)|((unsigned long)(code))))

/* ------------------------------------------------------------------ */
/*  MAKEWORD/MAKELONG etc                                              */
/* ------------------------------------------------------------------ */

#define MAKEWORD(a,b)   ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | \
                                 ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a,b)   ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | \
                                 ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)       ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)       ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)       ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)       ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

/* ------------------------------------------------------------------ */
/*  Struct packing & public API                                        */
/* ------------------------------------------------------------------ */

#ifndef IN
#   define IN
#endif
#ifndef OUT
#   define OUT
#endif
#ifndef OPTIONAL
#   define OPTIONAL
#endif

#define DECLARE_HANDLE(n) typedef struct n##__ { int unused; } *n
DECLARE_HANDLE(HLOCAL);

/* Some very generic Win32 structure forward declarations. */

typedef struct tagPOINT { LONG x; LONG y; } POINT;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE;

typedef void (*THREAD_START_ROUTINE)(LPVOID);

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_WINDEF_H_ */
