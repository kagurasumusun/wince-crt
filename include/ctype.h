/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * ctype.h -- character classification/conversion.
 */
#ifndef _AKARI_CTYPE_H_
#define _AKARI_CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int isascii(int c);
int toascii(int c);
int tolower(int c);
int toupper(int c);
int _tolower(int c);
int _toupper(int c);
int __isascii(int c);
int __toascii(int c);
int iscsym(int c);
int iscsymf(int c);

#define _U  0x01    /* upper */
#define _L  0x02    /* lower */
#define _N  0x04    /* digit */
#define _S  0x08    /* whitespace */
#define _P  0x10    /* punctuation */
#define _C  0x20    /* control */
#define _X  0x40    /* hex digit */
#define _B  0x80    /* blank */

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_CTYPE_H_ */
