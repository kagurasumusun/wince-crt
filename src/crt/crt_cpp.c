/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * crt_cpp_support.c -- minimal C++ runtime helpers needed to link
 * C++ code compiled with clang for ARM Windows CE. Pure C; contains
 * no code from any C++ runtime library.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <akari/compiler.h>

/* Atexit support for C++ static destructors registered during .init_array */
void __cxa_atexit(void (*dtor)(void *), void *obj, void *dso)
{
    /* register with our atexit system later; for now keep a no-op */
    (void)dtor; (void)obj; (void)dso;
}
void __cxa_pure_virtual(void) { for(;;){} }
void __cxa_guard_acquire(void) {}
void __cxa_guard_release(void) {}
void __cxa_guard_abort(void) {}

/*
 * Operator new/delete as weak fallbacks.
 *
 * We provide the Itanium/C++ ABI mangled names so code compiled with
 * `-fms-compatibility` or full `-fms-extensions` can link. Clang for
 * Windows/ARM (coredll/PE) emits calls to these mangled symbols.
 *
 * Note: C++ "operator new" under MS ABI has two forms of delete
 * (size-aware "sized delete" since C++14). We provide both.
 *
 * The names below are produced by the assembler .set directive so that
 * the C bodies (`__akari_op_new`, etc.) are exposed under the mangled
 * identifiers without requiring a separate .cpp translation unit.
 */
static void *__akari_op_new(size_t n)           { return malloc(n ? n : 1); }
static void  __akari_op_del(void *p)           { if (p) free(p); }
static void *__akari_op_new_nothrow(size_t n)  { return __akari_op_new(n); }
static void  __akari_op_del_nothrow(void *p)   { __akari_op_del(p); }
static void *__akari_op_new_arr(size_t n)      { return __akari_op_new(n); }
static void  __akari_op_del_arr(void *p)       { __akari_op_del(p); }
static void  __akari_op_del_sized(void *p, size_t n) { (void)n; __akari_op_del(p); }
static void  __akari_op_del_arr_sized(void *p, size_t n) { (void)n; __akari_op_del(p); }

/*
 * MSVC-style mangled names for ARM/PE/COFF (cfront-mangling):
 *   ??2@YAPAXI@Z  = operator new(unsigned int)
 *   ??3@YAXPAX@Z  = operator delete(void *)
 *   ??_U@YAPAXI@Z = operator new[](unsigned int)
 *   ??_V@YAXPAX@Z = operator delete[](void *)
 * Plus the Itanium aliases as weak for compatibility with clang in
 * non-MS-mode: _Znwj, _ZdlPv, etc.
 */
#if defined(_M_ARM) || defined(_M_ARM_NT) || defined(__arm__)
/* ARM name decoration: plain "?", prefix */
#  define AKARI_MANGLE_NEW       "??2@YAPAXI@Z"
#  define AKARI_MANGLE_DEL       "??3@YAXPAX@Z"
#  define AKARI_MANGLE_NEW_ARR   "??_U@YAPAXI@Z"
#  define AKARI_MANGLE_DEL_ARR   "??_V@YAXPAX@Z"
#else
/* x86 __cdecl uses a slightly different mangling */
#  define AKARI_MANGLE_NEW       "??2@YAPAXI@Z"
#  define AKARI_MANGLE_DEL       "??3@YAXPAX@Z"
#  define AKARI_MANGLE_NEW_ARR   "??_U@YAPAXI@Z"
#  define AKARI_MANGLE_DEL_ARR   "??_V@YAXPAX@Z"
#endif

#ifndef _DEBUG_HOSTCHECK_
/*
 * PE/COFF symbol aliases for operator new/delete under MSVC name
 * mangling. These aliases are only needed for real ARM-PE builds, so
 * we emit them only when building for the target, not the host.
 *
 * The weak aliases are created via the __asm__ .set directive, which
 * lld accepts for COFF targets. GNU as on Linux will not accept the
 * '?'-prefixed names; that's fine, they are not needed for hostcheck.
 */
void *_akari_new_wrap(size_t n) { return __akari_op_new(n); }
void  _akari_del_wrap(void *p)  { __akari_op_del(p); }
void *_akari_new_arr_wrap(size_t n) { return __akari_op_new_arr(n); }
void  _akari_del_arr_wrap(void *p)  { __akari_op_del_arr(p); }
void  _akari_del_sized_wrap(void *p, size_t n) { __akari_op_del_sized(p,n); }
void  _akari_del_arr_sized_wrap(void *p, size_t n) { __akari_op_del_arr_sized(p,n); }

void *operator_new(size_t n) __attribute__((alias("_akari_new_wrap"), weak));
void  operator_delete(void *p) __attribute__((alias("_akari_del_wrap"), weak));
void *operator_new_arr(size_t n) __attribute__((alias("_akari_new_arr_wrap"), weak));
void  operator_delete_arr(void *p) __attribute__((alias("_akari_del_arr_wrap"), weak));

__asm__(".set " AKARI_MANGLE_NEW ", _akari_new_wrap\n"
        ".set " AKARI_MANGLE_DEL ", _akari_del_wrap\n"
        ".set " AKARI_MANGLE_NEW_ARR ", _akari_new_arr_wrap\n"
        ".set " AKARI_MANGLE_DEL_ARR ", _akari_del_arr_wrap\n");

/* Itanium ABI aliases */
__asm__(".set _Znwj, _akari_new_wrap\n"
        ".set _Znwm, _akari_new_wrap\n"
        ".set _ZdlPv, _akari_del_wrap\n"
        ".set _ZdlPvj, _akari_del_sized_wrap\n"
        ".set _ZdaPv, _akari_del_arr_wrap\n"
        ".set _ZnaPv, _akari_new_arr_wrap\n");
#endif

/*
 * C++ EH personalities for MSVC-style SEH on Windows on ARM.
 * The real __CxxFrameHandler3 (in coredll / msvcrt) does the personality
 * dispatch; providing a weak fallback that simply aborts avoids link
 * errors when coredll import is missing or for host builds.
 */
void __CxxFrameHandler(void)         { for(;;){} }
void __CxxFrameHandler3(void)        { for(;;){} }
void _CxxThrowException(void *obj, void *info) { (void)obj; (void)info; for(;;){} }
void *__RTDynamicCast(const void *obj, int vf, void *src, void *dst, int hint) {
    (void)obj;(void)vf;(void)src;(void)dst;(void)hint; return NULL;
}
void *__RTtypeid(const void *obj) { (void)obj; return NULL; }
void _abnormal_termination(void) { for(;;){} }
void __std_terminate(void)       { for(;;){} }
void __std_terminate_id(void)    { for(;;){} }
void _purecall(void)             { for(;;){} }
void __purecall(void)            { for(;;){} }
void _invalid_parameter_noinfo(void) { for(;;){} }
void __CxxUnwind(void)           { for(;;){} }
void __DestructExceptionObject(void *a, void *b) { (void)a;(void)b; }
int  __CxxExceptionFilter(void *a,void *b,void *c) { (void)a;(void)b;(void)c; return 1; }
void __CxxLongjmpUnwind(void *a) { (void)a; for(;;){} }
void __CxxQueryExceptionSize(void) {}
extern void __chkstk(void);
/* Symbols expected by MSVC CRT */
int  __CppXcptFilter(unsigned long x, void *ep) {
    (void)x; (void)ep; return 1;
}

/* Stack protector */
#if defined(__SSP__) || defined(__SSP_ALL__)
uintptr_t __stack_chk_guard = 0x000a0dffUL;
NORETURN void __stack_chk_fail(void) { for(;;){} }
#endif

/* Compiler expects __chkstk to exist; provided in compiler-rt */
extern void __chkstk(void);

/* Atomic helpers stub (Clang may emit __sync_* builtins directly;
 * these are only pulled in if optimizations/atomics are disabled). */
