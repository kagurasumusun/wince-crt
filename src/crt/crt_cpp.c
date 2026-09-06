/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt_cpp.c -- C++ runtime glue needed to link C++ code built by
 * clang for ARM/PE/COFF Windows CE. These are purely ABI symbols:
 * operator new/delete (MS-mangled and Itanium-mangled weak aliases),
 * MSVC-style C++ EH personality stubs, guard-variable helpers for
 * function-local static initialisation, and a stack-chk guard.
 *
 * None of these are the "real" C++ standard library; that ships
 * separately (libc++ / libc++abi). The symbols here exist only so
 * that object files produced by clang don't fail at link time with
 * missing ABI helpers.
 */
#include <stddef.h>
#include <stdint.h>
#include <akari/compiler.h>

/* Provided by the consumer's C library */
extern void *malloc(unsigned long);
extern void  free(void *);

/* ---- atexit / __cxa_atexit (forwarded to misc/atexit.c) ---- */
extern int __cxa_atexit(void (*dtor)(void *), void *obj, void *dso);

/* ---- Guard variables for local static init (single-threaded CE) ---- */
int  __cxa_guard_acquire(uint64_t *g) { if (!g) return 0; return !*(volatile uint8_t*)g; }
void __cxa_guard_release(uint64_t *g) { if (g) *(volatile uint8_t*)g = 1; }
void __cxa_guard_abort(uint64_t *g)   { (void)g; }

/* ---- Pure-virtual / bad cast stubs ---- */
void __cxa_pure_virtual(void) { for(;;){} }
void _purecall(void)          { for(;;){} }
void __purecall(void)         { for(;;){} }

/* ---- Operator new/delete ---- */
static void *op_new(unsigned long n)         { return malloc(n ? n : 1); }
static void  op_del(void *p)                 { if (p) free(p); }
static void *op_new_arr(unsigned long n)     { return op_new(n); }
static void  op_del_arr(void *p)             { op_del(p); }
static void  op_del_sized(void *p, unsigned long n) { (void)n; op_del(p); }
static void  op_del_arr_sized(void *p, unsigned long n) { (void)n; op_del(p); }

/* Expose C-linkage wrappers with weak MS/Itanium aliases via __asm .set
 * (lld/COFF accepts these for PE targets). On non-COFF (host) builds
 * we skip the alias directives because the host assembler doesn't
 * accept '?' symbols. */
void *_akari_new_wrap(unsigned long n) { return op_new(n); }
void  _akari_del_wrap(void *p)         { op_del(p); }
void *_akari_new_arr_wrap(unsigned long n) { return op_new_arr(n); }
void  _akari_del_arr_wrap(void *p)      { op_del_arr(p); }
void  _akari_del_sized_wrap(void *p, unsigned long n) { op_del_sized(p,n); }
void  _akari_del_arr_sized_wrap(void *p, unsigned long n) { op_del_arr_sized(p,n); }

#if defined(_M_ARM) || defined(__arm__) || defined(__thumb__)
/* MS-mangled names for ARM/PE */
__asm__(".set ??2@YAPAXI@Z, _akari_new_wrap\n"
        ".set ??3@YAXPAX@Z, _akari_del_wrap\n"
        ".set ??_U@YAPAXI@Z, _akari_new_arr_wrap\n"
        ".set ??_V@YAXPAX@Z, _akari_del_arr_wrap\n");
/* Itanium */
__asm__(".set _Znwj, _akari_new_wrap\n"
        ".set _Znwm, _akari_new_wrap\n"
        ".set _ZdlPv, _akari_del_wrap\n"
        ".set _ZdlPvj, _akari_del_sized_wrap\n"
        ".set _ZdaPv, _akari_del_arr_wrap\n"
        ".set _ZnaPv, _akari_new_arr_wrap\n");
#endif

/* ---- C++ EH personalities (MSVC-style SEH on ARM) ----
 * The real implementations live in coredll / C++ runtime. We provide
 * weak fallbacks that simply abort so code that doesn't actually
 * throw will link. */
WEAK void __CxxFrameHandler3(void)        { for(;;){} }
WEAK void __CxxFrameHandler(void)         { for(;;){} }
WEAK void _CxxThrowException(void *obj, void *info) { (void)obj;(void)info; for(;;){} }
WEAK void *__RTDynamicCast(const void *obj, int vf, const void *src, const void *dst, int hint) {
    (void)obj;(void)vf;(void)src;(void)dst;(void)hint; return NULL;
}
WEAK void *__RTtypeid(const void *obj) { (void)obj; return NULL; }
WEAK void _abnormal_termination(void)  { for(;;){} }
WEAK void __std_terminate(void)        { for(;;){} }
WEAK void _invalid_parameter_noinfo(void) { for(;;){} }
WEAK void __CxxUnwind(void)            { for(;;){} }
WEAK void __DestructExceptionObject(void *a, void *b) { (void)a;(void)b; }
WEAK int  __CxxExceptionFilter(void *a,void *b,void *c) { (void)a;(void)b;(void)c; return 1; }
WEAK void __CxxLongjmpUnwind(void *a)  { (void)a; for(;;){} }
WEAK void __CxxQueryExceptionSize(void) {}

/* ---- Stack protector ---- */
#if defined(__SSP__) || defined(__SSP_ALL__)
uintptr_t __stack_chk_guard = 0x000a0dffUL;
NORETURN void __stack_chk_fail(void) { for(;;){} }
#endif

extern void __chkstk(void);   /* in compiler-rt / chkstk_arm.S */
