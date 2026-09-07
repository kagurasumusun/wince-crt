/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * internal.h -- declarations shared between the CRT translation units.
 * This is NOT a public header; consumers use <akari/crt.h>.
 *
 * akari_wchar is the native 16-bit Windows code unit (UTF-16 code
 * unit on Windows CE).  It is defined independently of the C
 * compiler's wchar_t so that the CRT can be compiled and self-tested
 * on non-Windows hosts (where wchar_t is 32-bit) without changing
 * semantics; on the Windows CE targets uint16_t and wchar_t are the
 * same type, so the public <akari/crt.h> declarations (which use
 * wchar_t) match these definitions exactly.
 */
#ifndef _AKARI_INTERNAL_H_
#define _AKARI_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

typedef uint16_t akari_wchar;
typedef uint32_t akari_dword;
typedef void    *akari_handle;

/* Data objects shared by EXE and DLL modules (see crt.h). */
extern int           __argc;
extern char        **__argv;
extern akari_wchar **__wargv;
extern char         *_acmdln;
extern akari_wchar  *_wcmdln;
extern akari_wchar  *_wcmdtail;
extern int           _fmode;
extern int           _doserrno;
extern int           _commode;
extern void         *__dso_handle;

/* Module handle of the current image (GetModuleHandleW(NULL)). */
akari_handle akari_image_handle(void);

/* Parse the wide command line into __argc/__argv/__wargv, compute
 * _wcmdtail, synthesize the narrow forms.  Called once, from the EXE
 * entry points, before any user code runs. */
void akari_init_args(void);

/* Run global initializers in the documented order:
 *   .CRT$XI* (C) and .CRT$XC* (C++) first-to-last (link order),
 *   then the lld GNU __CTOR_LIST__ walked backward (per-object
 *   source order; objects in reverse link order, the historical
 *   GNU .ctors convention -- see runtime.c layout notes). */
void akari_run_ctors(void);

/* Run global destructors from the lld GNU __DTOR_LIST__.  Words are
 * stored per object in reverse source order and objects are
 * concatenated in link order, so walking the list forward runs
 * destructors in the exact reverse of the constructor order
 * (last-constructed first), both inside one object and across
 * objects.  Used by the DLL path on DLL_PROCESS_DETACH and by the
 * EXE path before the libc exit() hand-off. */
void akari_run_dtors(void);

#endif /* _AKARI_INTERNAL_H_ */
