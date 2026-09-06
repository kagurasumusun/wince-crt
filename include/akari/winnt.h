/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * akari/winnt.h -- Win32 / WinCE NT-level constants used by the runtime.
 */
#ifndef _AKARI_WINNT_H_
#define _AKARI_WINNT_H_

#include <akari/windef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Page/Protection/Memory constants                                   */
/* ------------------------------------------------------------------ */

#define MEM_COMMIT              0x00001000
#define MEM_RESERVE             0x00002000
#define MEM_DECOMMIT            0x00004000
#define MEM_RELEASE             0x00008000
#define MEM_FREE                0x00010000
#define MEM_TOP_DOWN            0x00100000
#define MEM_LARGE_PAGES         0x20000000

#define PAGE_READONLY           0x02
#define PAGE_READWRITE          0x04
#define PAGE_WRITECOPY          0x08
#define PAGE_EXECUTE            0x10
#define PAGE_EXECUTE_READ       0x20
#define PAGE_EXECUTE_READWRITE  0x40
#define PAGE_GUARD              0x100

#define LMEM_FIXED              0x0000
#define LMEM_MOVEABLE           0x0002
#define LMEM_ZEROINIT           0x0040
#define LPTR                    (LMEM_FIXED | LMEM_ZEROINIT)
#define LHND                    (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define NONZEROLHND             (LMEM_MOVEABLE)
#define NONZEROLPTR             (LMEM_FIXED)

#define LocalDiscard(l)         (LocalReAlloc((l), 0, LMEM_MOVEABLE))
#define LocalLock(l)            ((LPVOID)(l))  /* handles are pointers in CE */
#define LocalUnlock(l)          (TRUE)

/* ------------------------------------------------------------------ */
/*  DLL entry-point reasons                                            */
/* ------------------------------------------------------------------ */

#define DLL_PROCESS_ATTACH      1
#define DLL_THREAD_ATTACH       2
#define DLL_THREAD_DETACH       3
#define DLL_PROCESS_DETACH      0

/* ------------------------------------------------------------------ */
/*  Standard handles / limits                                          */
/* ------------------------------------------------------------------ */

#define INVALID_HANDLE_VALUE    ((HANDLE)(LONG_PTR)-1)
#define INVALID_FILE_SIZE       ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

#define MAX_PATH                260

/* ------------------------------------------------------------------ */
/*  Wait / Thread constants                                            */
/* ------------------------------------------------------------------ */

#define INFINITE                0xFFFFFFFF
#define WAIT_OBJECT_0           ((STATUS_WAIT_0) + 0)
#define WAIT_TIMEOUT            258L
#define STATUS_WAIT_0           ((DWORD)0x00000000)

#define CREATE_SUSPENDED        0x00000004
#define CREATE_NEW_CONSOLE      0x00000010  /* not present on CE */

#define STILL_ACTIVE            STATUS_PENDING
#define STATUS_PENDING          0x00000103L

/* ------------------------------------------------------------------ */
/*  Access rights                                                      */
/* ------------------------------------------------------------------ */

#define DELETE                  0x00010000L
#define READ_CONTROL            0x00020000L
#define WRITE_DAC               0x00040000L
#define WRITE_OWNER             0x00080000L
#define SYNCHRONIZE             0x00100000L
#define STANDARD_RIGHTS_READ    READ_CONTROL
#define STANDARD_RIGHTS_WRITE   READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE READ_CONTROL
#define STANDARD_RIGHTS_REQUIRED 0x000F0000L

/* ------------------------------------------------------------------ */
/*  Access masks for files                                             */
/* ------------------------------------------------------------------ */

#define GENERIC_READ            (0x80000000L)
#define GENERIC_WRITE           (0x40000000L)
#define GENERIC_EXECUTE         (0x20000000L)
#define GENERIC_ALL             (0x10000000L)

#define FILE_SHARE_READ         0x00000001
#define FILE_SHARE_WRITE        0x00000002

#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN   0x00000002
#define FILE_ATTRIBUTE_SYSTEM   0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE  0x00000020
#define FILE_ATTRIBUTE_NORMAL   0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100

#define INVALID_HANDLE_VALUE    ((HANDLE)(LONG_PTR)-1)

/* ------------------------------------------------------------------ */
/*  File seek / stdio                                                  */
/* ------------------------------------------------------------------ */

#define STD_INPUT_HANDLE        ((DWORD)-10)
#define STD_OUTPUT_HANDLE       ((DWORD)-11)
#define STD_ERROR_HANDLE        ((DWORD)-12)

/* ------------------------------------------------------------------ */
/*  Registry                                                           */
/* ------------------------------------------------------------------ */

#define HKEY_LOCAL_MACHINE      ((HKEY)(ULONG_PTR)((LONG)0x80000002))
#define HKEY_CURRENT_USER       ((HKEY)(ULONG_PTR)((LONG)0x80000001))
#define HKEY_CLASSES_ROOT       ((HKEY)(ULONG_PTR)((LONG)0x80000000))

/* ------------------------------------------------------------------ */
/*  Window messages                                                    */
/* ------------------------------------------------------------------ */

#define WM_NULL                 0x0000
#define WM_DESTROY              0x0002
#define WM_CLOSE                0x0010
#define WM_QUIT                 0x0012
#define WM_PAINT                0x000F
#define WM_COMMAND              0x0111

#define SW_HIDE                 0
#define SW_SHOWNORMAL           1
#define SW_SHOW                 5
#define SW_SHOWMAXIMIZED        3
#define SW_SHOWMINIMIZED        2

#define MB_OK                   0x00000000L
#define MB_OKCANCEL             0x00000001L
#define MB_ICONERROR            0x00000010L
#define MB_ICONWARNING          0x00000030L
#define MB_ICONINFORMATION      0x00000040L

#define IDOK                    1
#define IDCANCEL                2

/* ------------------------------------------------------------------ */
/*  Coredll / Kernel APIs declared as externs (resolved via import lib) */
/* ------------------------------------------------------------------ */

/* Memory */
HLOCAL  WINAPI LocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL  WINAPI LocalReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags);
HLOCAL  WINAPI LocalFree(HLOCAL hMem);
UINT    WINAPI LocalSize(HLOCAL hMem);
HANDLE  WINAPI GetProcessHeap(void);
LPVOID  WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes);
LPVOID  WINAPI HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, DWORD dwBytes);
BOOL    WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
DWORD   WINAPI GetLastError(void);
void    WINAPI SetLastError(DWORD dwErrCode);

/* Process / module */
void    WINAPI ExitProcess(UINT uExitCode);
void    WINAPI Sleep(DWORD dwMilliseconds);
HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);
LPWSTR  WINAPI GetCommandLineW(void);
void    WINAPI OutputDebugStringW(LPCWSTR lpOutputString);
void    WINAPI OutputDebugStringA(LPCSTR lpOutputString);

/* File I/O */
HANDLE  WINAPI CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess,
                           DWORD dwShareMode, LPVOID lpSecurityAttributes,
                           DWORD dwCreationDisposition,
                           DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL    WINAPI ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped);
BOOL    WINAPI WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                         LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped);
BOOL    WINAPI CloseHandle(HANDLE hObject);
DWORD   WINAPI SetFilePointer(HANDLE hFile, LONG lDistanceToMove,
                              PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
DWORD   WINAPI GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
BOOL    WINAPI SetEndOfFile(HANDLE hFile);

/* Time */
void    WINAPI GetSystemTime(LPVOID lpSystemTime);
DWORD   WINAPI GetTickCount(void);

/* Wide <-> Ansi conversion helpers (not available on CE; provided by us) */
/* These are actually only on desktop Win32. We implement them ourselves in
 * src/mbstring.c if desired. */

/* Dynamic library */
FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
HMODULE WINAPI LoadLibraryW(LPCWSTR lpLibFileName);
BOOL    WINAPI FreeLibrary(HMODULE hLibModule);

/* Threading */
HANDLE  WINAPI CreateThread(LPVOID lpThreadAttributes, SIZE_T dwStackSize,
                            THREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                            DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD   WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
HANDLE  WINAPI CreateEventW(LPVOID lpEventAttributes, BOOL bManualReset,
                            BOOL bInitialState, LPCWSTR lpName);
BOOL    WINAPI SetEvent(HANDLE hEvent);
BOOL    WINAPI ResetEvent(HANDLE hEvent);

/* Critical section */
typedef struct _CRITICAL_SECTION {
    DWORD   DebugInfo;
    LONG    LockCount;
    LONG    RecursionCount;
    HANDLE  OwningThread;
    HANDLE  LockSemaphore;
    DWORD   SpinCount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;
void    WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void    WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void    WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void    WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
BOOL    WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

/* Tls */
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
DWORD   WINAPI TlsAlloc(void);
BOOL    WINAPI TlsFree(DWORD dwTlsIndex);
LPVOID  WINAPI TlsGetValue(DWORD dwTlsIndex);
BOOL    WINAPI TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);

/* Message box (GUI helper) */
int     WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
int     WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

/* Misc */
void    WINAPI DebugBreak(void);
void    WINAPI FatalExit(int code);
DWORD   WINAPI GetCurrentProcessId(void);
DWORD   WINAPI GetCurrentThreadId(void);

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_WINNT_H_ */
