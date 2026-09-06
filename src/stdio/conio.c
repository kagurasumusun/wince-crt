/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <akari/compiler.h>

int _getch(void){return -1;}
int _getche(void){return -1;}
int _putch(int ch){(void)ch;return ch;}
int _kbhit(void){return 0;}
int _cputs(const char *s){(void)s;return 0;}
int _ungetch(int ch){(void)ch;return ch;}
