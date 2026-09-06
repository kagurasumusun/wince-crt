/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <wctype.h>
#include <wchar.h>

extern const unsigned short *_akari_ctype_wide(void);

static const unsigned short _akari_wc_ctype[] = {
    [0x00]=_CNTRL,[0x01]=_CNTRL,[0x02]=_CNTRL,[0x03]=_CNTRL,
    [0x04]=_CNTRL,[0x05]=_CNTRL,[0x06]=_CNTRL,[0x07]=_CNTRL,
    [0x08]=_CNTRL,[0x09]=_SPACE|_BLANK,[0x0A]=_SPACE,[0x0B]=_SPACE,
    [0x0C]=_SPACE,[0x0D]=_SPACE,[0x0E]=_CNTRL,[0x0F]=_CNTRL,
    [0x10]=_CNTRL,[0x11]=_CNTRL,[0x12]=_CNTRL,[0x13]=_CNTRL,
    [0x14]=_CNTRL,[0x15]=_CNTRL,[0x16]=_CNTRL,[0x17]=_CNTRL,
    [0x18]=_CNTRL,[0x19]=_CNTRL,[0x1A]=_CNTRL,[0x1B]=_CNTRL,
    [0x1C]=_CNTRL,[0x1D]=_CNTRL,[0x1E]=_CNTRL,[0x1F]=_CNTRL,
    [0x20]=_SPACE|_BLANK|_PRINT,
    [0x21]=_PUNCT|_PRINT,[0x22]=_PUNCT|_PRINT,[0x23]=_PUNCT|_PRINT,
    [0x24]=_PUNCT|_PRINT,[0x25]=_PUNCT|_PRINT,[0x26]=_PUNCT|_PRINT,
    [0x27]=_PUNCT|_PRINT,[0x28]=_PUNCT|_PRINT,[0x29]=_PUNCT|_PRINT,
    [0x2A]=_PUNCT|_PRINT,[0x2B]=_PUNCT|_PRINT,[0x2C]=_PUNCT|_PRINT,
    [0x2D]=_PUNCT|_PRINT,[0x2E]=_PUNCT|_PRINT,[0x2F]=_PUNCT|_PRINT,
};

static int _wtyp(wint_t c, unsigned mask)
{
    if (c < 128) {
        if (c < 0x80 && _akari_wc_ctype[c] & mask) return 1;
        return 0;
    }
    /* Latin-1: alpha upper 0xC0-0xD6, 0xD8-0xDE; lower 0xDF-0xF6, 0xF8-0xFE */
    if (c >= 0xC0 && c <= 0xD6) return (mask & _UPPER) != 0;
    if (c >= 0xD8 && c <= 0xDE) return (mask & _UPPER) != 0;
    if (c >= 0xDF && c <= 0xF6) return (mask & _LOWER) != 0;
    if (c >= 0xF8 && c <= 0xFE) return (mask & _LOWER) != 0;
    if (c == 0xA0 || c == 0xAD) return (mask & _SPACE) != 0;
    return 0;
}

int iswalnum(wint_t c){return _wtyp(c,_ALNUM);}
int iswalpha(wint_t c){return _wtyp(c,_ALPHA);}
int iswblank(wint_t c){return _wtyp(c,_BLANK);}
int iswcntrl(wint_t c){return _wtyp(c,_CNTRL);}
int iswdigit(wint_t c){return c>=L'0'&&c<=L'9';}
int iswgraph(wint_t c){return _wtyp(c,_PUNCT|_ALNUM);}
int iswlower(wint_t c){return _wtyp(c,_LOWER);}
int iswprint(wint_t c){return c>=0x20&&c<0x7F;}
int iswpunct(wint_t c){return _wtyp(c,_PUNCT);}
int iswspace(wint_t c){return _wtyp(c,_SPACE);}
int iswupper(wint_t c){return _wtyp(c,_UPPER);}
int iswxdigit(wint_t c){return (c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F');}

wint_t towlower(wint_t c)
{
    if (c>=L'A'&&c<=L'Z') return c + (L'a'-L'A');
    if (c>=0xC0&&c<=0xDE&&c!=0xD7) return c + 0x20;
    return c;
}
wint_t towupper(wint_t c)
{
    if (c>=L'a'&&c<=L'z') return c - (L'a'-L'A');
    if (c>=0xDF&&c<=0xFE&&c!=0xF7) return c - 0x20;
    return c;
}

int iswctype(wint_t wc, wctype_t desc){ (void)desc; return iswalnum(wc); }
wctype_t wctype(const char *name){(void)name;return 0;}
wctrans_t wctrans(const char *name){(void)name;return 0;}
wint_t towctrans(wint_t wc, wctrans_t d){(void)d;return wc;}
