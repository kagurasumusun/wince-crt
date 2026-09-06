/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * ctype.c -- character classification & case conversion.
 *
 * Implementation is based solely on ISO C semantics; no external code used.
 */
#include <ctype.h>
#include <wctype.h>

/*
 * 0x00-0x7F only; characters >=0x80 return 0 for the locale-agnostic C locale.
 *
 * Bits: _C(0x20)=control, _P(0x10)=punct, _N(0x04)=digit,
 *       _U(0x01)=upper, _L(0x02)=lower, _S(0x08)=space,
 *       _X(0x40)=xdigit, _B(0x80)=blank
 */
static const unsigned char _ctype_[] = {
    0x00,                          /* -1 EOF */
    _C, _C, _C, _C, _C, _C, _C, _C,     /* 0x00-0x07 */
    _C, _C|_S|_B, _C|_S, _C|_S, _C|_S, _C|_S, _C, _C, /* 0x08-0x0F */
    _C, _C, _C, _C, _C, _C, _C, _C,     /* 0x10-0x17 */
    _C, _C, _C, _C, _C, _C, _C, _C,     /* 0x18-0x1F */
    _S|_B, _P, _P, _P, _P, _P, _P, _P,  /* 0x20: ' '   !"#$%&' */
    _P, _P, _P, _P, _P, _P, _P, _P,     /* 0x28: ()*+,-./ */
    _N|_X, _N|_X, _N|_X, _N|_X, _N|_X,  /* 0x30: 0-4 */
    _N|_X, _N|_X, _N|_X, _N|_X, _N|_X, _P, _P, _P,  /* 0x35: 5-9 :;<=> */
    _P, _P, _P,                         /* 0x3E: >?@ */
    _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U|_X, _U, /* 0x41: A-F */
    _U, _U, _U, _U, _U, _U, _U, _U,     /* G-O */
    _U, _U, _U, _U, _U, _U, _U, _U,     /* P-W */
    _U, _U, _U, _P, _P, _P, _P, _P,     /* X-Z [\]^_ */
    _P, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L|_X, _L, /* 0x61: `a-f */
    _L, _L, _L, _L, _L, _L, _L, _L,     /* g-o */
    _L, _L, _L, _L, _L, _L, _L, _L,     /* p-w */
    _L, _L, _L, _P, _P, _P, _P, _C,     /* x-z {|}~ DEL */
};

int isalnum(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & (_U|_L|_N)); }
int isalpha(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & (_U|_L)); }
int isblank(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _B); }
int iscntrl(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _C); }
int isdigit(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _N); }
int isgraph(int c)  { return (unsigned)c >= 0x21 && (unsigned)c <= 0x7e; }
int islower(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _L); }
int isprint(int c)  { return (unsigned)c >= 0x20 && (unsigned)c <= 0x7e; }
int ispunct(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _P); }
int isspace(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _S); }
int isupper(int c)  { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _U); }
int isxdigit(int c) { return (unsigned)c <= 0x7f && (_ctype_[c+1] & _X); }
int isascii(int c)  { return (unsigned)c <= 0x7f; }
int toascii(int c)  { return c & 0x7f; }

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}
int toupper(int c)
{
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

/* Wide-char ctype (C-locale only) */
int iswalnum(wint_t c)  { return isalnum((int)c); }
int iswalpha(wint_t c)  { return isalpha((int)c); }
int iswblank(wint_t c)  { return isblank((int)c); }
int iswcntrl(wint_t c)  { return iscntrl((int)c); }
int iswdigit(wint_t c)  { return isdigit((int)c); }
int iswgraph(wint_t c)  { return isgraph((int)c); }
int iswlower(wint_t c)  { return islower((int)c); }
int iswprint(wint_t c)  { return isprint((int)c); }
int iswpunct(wint_t c)  { return ispunct((int)c); }
int iswspace(wint_t c)  { return isspace((int)c); }
int iswupper(wint_t c)  { return isupper((int)c); }
int iswxdigit(wint_t c) { return isxdigit((int)c); }
wint_t towlower(wint_t c){ return tolower((int)c); }
wint_t towupper(wint_t c){ return toupper((int)c); }
int iswctype(wint_t c, wctype_t d){ (void)d; return iswalnum(c); }
wctype_t wctype(const char *n){ (void)n; return 0; }
wctrans_t wctrans(const char *n){ (void)n; return 0; }
wint_t towctrans(wint_t c, wctrans_t d){ (void)d; return c; }

int _tolower(int c){ return c >= 'A' && c <= 'Z' ? c + ('a'-'A') : c; }
int _toupper(int c){ return c >= 'a' && c <= 'z' ? c - ('a'-'A') : c; }
int __isascii(int c){ return isascii(c); }
int __toascii(int c){ return toascii(c); }
int iscsym(int c){ return isalnum(c) || c == '_'; }
int iscsymf(int c){ return isalpha(c) || c == '_'; }
