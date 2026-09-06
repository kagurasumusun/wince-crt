/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_ASSERT_H_
#define _AKARI_ASSERT_H_
#include <akari/compiler.h>
#ifdef __cplusplus
extern "C" {
#endif
void _akari_assert_fail(const char *expr, const char *file, int line, const char *func) NORETURN;

#ifdef NDEBUG
#   define assert(ignore) ((void)0)
#else
#   if defined(__GNUC__) || defined(__clang__)
#       define assert(e) ((e) ? (void)0 : _akari_assert_fail(#e, __FILE__, __LINE__, __func__))
#   else
#       define assert(e) ((e) ? (void)0 : _akari_assert_fail(#e, __FILE__, __LINE__, 0))
#   endif
#endif

#ifdef __cplusplus
}
#endif
#endif
