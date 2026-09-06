/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>

char *itoa(int v, char *s, int radix)
{
    char buf[34], *p = buf + sizeof(buf) - 1;
    int neg = 0; unsigned u;
    *p = '\0';
    if (v < 0 && radix == 10) { neg = 1; u = (unsigned)(-v); } else u = (unsigned)v;
    if (u == 0) *--p = '0';
    else while (u) { *--p = "0123456789abcdef"[u % radix]; u /= radix; }
    if (neg) *--p = '-';
    { char *o = s; while ((*o++ = *p++)) {} return s; }
}

char *ltoa(long v, char *s, int radix)
{
    char buf[68], *p = buf + sizeof(buf)-1; int neg=0; unsigned long u; *p='\0';
    if (v < 0 && radix == 10) { neg=1; u=(unsigned long)(-v);} else u=(unsigned long)v;
    if (u==0) *--p='0';
    else while(u){*--p = "0123456789abcdef"[u%radix]; u/=radix;}
    if (neg) *--p='-';
    {char *o=s; while((*o++=*p++));return s;}
}

char *ultoa(unsigned long v, char *s, int radix)
{
    char buf[68], *p=buf+sizeof(buf)-1; *p='\0';
    if (!v) *--p='0';
    else while(v){*--p = "0123456789abcdef"[v%radix]; v/=radix;}
    {char *o=s;while((*o++=*p++));return s;}
}

char *i64toa(long long v, char *s, int radix)
{
    char buf[68], *p=buf+sizeof(buf)-1; int neg=0; unsigned long long u; *p='\0';
    if (v<0 && radix==10){neg=1;u=(unsigned long long)(-v);} else u=(unsigned long long)v;
    if(!u)*--p='0';
    else while(u){*--p="0123456789abcdef"[u%radix];u/=radix;}
    if(neg)*--p='-';
    {char *o=s;while((*o++=*p++));return s;}
}

char *ui64toa(unsigned long long v, char *s, int radix)
{
    char buf[68], *p=buf+sizeof(buf)-1;*p='\0';
    if(!v)*--p='0';
    else while(v){*--p="0123456789abcdef"[v%radix];v/=radix;}
    {char *o=s;while((*o++=*p++));return s;}
}

/* Wide versions */
wchar_t *_itow(int v, wchar_t *s, int radix)
{
    char tmp[34]; itoa(v,tmp,radix); wchar_t *o=s; char *t=tmp; while((*o++=(wchar_t)(unsigned char)*t++)); return s;
}
wchar_t *_ltow(long v, wchar_t *s, int radix)
{ char tmp[68]; ltoa(v,tmp,radix); wchar_t *o=s; char *t=tmp; while((*o++=(wchar_t)(unsigned char)*t++)); return s;}
wchar_t *_ultow(unsigned long v, wchar_t *s, int radix)
{ char tmp[68]; ultoa(v,tmp,radix); wchar_t *o=s; char *t=tmp; while((*o++=(wchar_t)(unsigned char)*t++)); return s;}
wchar_t *_i64tow(long long v, wchar_t *s, int radix)
{ char tmp[68]; i64toa(v,tmp,radix); wchar_t *o=s; char *t=tmp; while((*o++=(wchar_t)(unsigned char)*t++)); return s;}
wchar_t *_ui64tow(unsigned long long v, wchar_t *s, int radix)
{ char tmp[68]; ui64toa(v,tmp,radix); wchar_t *o=s; char *t=tmp; while((*o++=(wchar_t)(unsigned char)*t++)); return s;}

char *strupr(char *s){for(char *p=s;*p;p++)if(*p>='a'&&*p<='z')*p-=32;return s;}
char *strlwr(char *s){for(char *p=s;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;return s;}
char *strset(char *s, int c){char *p=s;while(*p)*p++=(char)c;return s;}
char *strnset(char *s, int c, size_t n){char *p=s;while(n--&&*p)*p++=(char)c;return s;}
char *strrev(char *s){size_t n=strlen(s);for(size_t i=0;i<n/2;i++){char t=s[i];s[i]=s[n-1-i];s[n-1-i]=t;}return s;}



/* Underscored aliases */
char *_itoa(int v, char *s, int r){return itoa(v,s,r);}
char *_ltoa(long v, char *s, int r){return ltoa(v,s,r);}
char *_ultoa(unsigned long v, char *s, int r){return ultoa(v,s,r);}
char *_i64toa(long long v, char *s, int r){return i64toa(v,s,r);}
char *_ui64toa(unsigned long long v, char *s, int r){return ui64toa(v,s,r);}
wchar_t *_itow(int v, wchar_t *s, int r);
wchar_t *_ltow(long v, wchar_t *s, int r);
wchar_t *_ultow(unsigned long v, wchar_t *s, int r);
wchar_t *_i64tow(long long v, wchar_t *s, int r);
wchar_t *_ui64tow(unsigned long long v, wchar_t *s, int r);
