/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_CONIO_H_
#define _AKARI_CONIO_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int _getch(void);
int _getche(void);
int _putch(int ch);
int _kbhit(void);
int _cputs(const char *s);
int _ungetch(int ch);

#ifdef __cplusplus
}
#endif
#endif
