/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * test_basic.c -- host-side sanity checks for portable C routines.
 * Builds against the same object files used for the cross target,
 * compiled with -D_DEBUG_HOSTCHECK_ so sources take the host-safe
 * backend (no coredll references; raw write/exit syscalls).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>
#include <signal.h>
#include <wchar.h>

void OutputDebugStringA(const char *s){ fputs(s, stderr); }
void OutputDebugStringW(const unsigned short *s){ (void)s; }

#ifdef _DEBUG_HOSTCHECK_
extern long write(int, const void*, unsigned long);
#define FAILMSG(s) do{const char*_m=s;long _n=0;while(_m[_n])_n++;write(2,"F:",2);write(2,_m,_n);write(2,"\n",1);}while(0)
#else
#define FAILMSG(s) ((void)0)
#endif
#define CHECK(cond) do { if (!(cond)) { fail++; } else { ok++; } } while(0)

static int fail = 0, ok = 0;

static int cmp_int(const void *a, const void *b)
{ return *(const int*)a - *(const int*)b; }

int main(void)
{
    /* memset / memcpy / memcmp */
    char buf[32];
    memset(buf, 'A', 32);
    CHECK(buf[0] == 'A' && buf[31] == 'A');
    memcpy(buf, "hello", 5);
    CHECK(memcmp(buf, "hello", 5) == 0);
    CHECK(memcmp(buf, "helloo", 6) != 0);
    CHECK(memchr(buf, 'o', 32) == buf + 4);

    /* strlen / strcpy / strcat / strcmp */
    char s1[64] = "abc";
    CHECK(strlen(s1) == 3);
    strcpy(s1, "0123456789");
    CHECK(strlen(s1) == 10);
    strcat(s1, "X");
    CHECK(strlen(s1) == 11);
    CHECK(s1[10] == 'X');
    CHECK(strcmp("abc", "abc") == 0);
    CHECK(strcmp("abc", "abd") < 0);
    CHECK(strcmp("ab", "abc") < 0);

    /* strncmp / strncpy */
    char s2[16];
    strncpy(s2, "test", sizeof(s2));
    CHECK(strncmp(s2, "test", 4) == 0);
    CHECK(s2[4] == '\0');

    /* strchr / strrchr / strstr */
    CHECK(strchr("hello", 'l') != NULL);
    CHECK(strrchr("hello", 'l') == strchr("hello", 'l') + 1);
    CHECK(strstr("hello world", "world") != NULL);
    CHECK(strstr("hello", "xyz") == NULL);

    /* strtol */
    char *end = NULL;
    CHECK(strtol("1234", &end, 10) == 1234);
    CHECK(*end == '\0');
    CHECK(strtol("  -42xyz", &end, 10) == -42);
    CHECK(*end == 'x');
    CHECK(strtol("0x10", NULL, 0) == 16);
    CHECK(strtoul("FF", NULL, 16) == 255);

    /* malloc / free / realloc / calloc */
    void *p = malloc(100);
    CHECK(p != NULL);
    memset(p, 0, 100);
    void *p2 = realloc(p, 500);
    CHECK(p2 != NULL);
    free(p2);
    void *pc = calloc(10, 8);
    CHECK(pc != NULL);
    { int i; char *cc = (char*)pc; for (i = 0; i < 80; i++) CHECK(cc[i] == 0); }
    free(pc);

    /* qsort / bsearch */
    int arr[] = { 5, 2, 8, 1, 9, 3 };
    qsort(arr, 6, sizeof(int), cmp_int);
    CHECK(arr[0] == 1 && arr[5] == 9 && arr[2] == 3);
    int key = 5;
    int *bp = (int *)bsearch(&key, arr, 6, sizeof(int), cmp_int);
    CHECK(bp != NULL && *bp == 5);

    /* rand/srand determinism */
    srand(1);
    int r1 = rand();
    srand(1);
    CHECK(rand() == r1);

    /* sprintf / snprintf */
    char out[64];
    sprintf(out, "%d+%d=%s", 2, 3, "5");
    CHECK(strcmp(out, "2+3=5") == 0);
    snprintf(out, 8, "%s", "abcdefghij");
    CHECK(strcmp(out, "abcdefg") == 0);
    sprintf(out, "0x%x", 255);
    CHECK(strcmp(out, "0xff") == 0);
    sprintf(out, "%04d", 7);
    CHECK(strcmp(out, "0007") == 0);

    /* abs */
    CHECK(abs(-42) == 42);
    CHECK(abs(42) == 42);

    /* ctype */
    CHECK(isdigit('5'));
    CHECK(!isdigit('a'));
    CHECK(isalpha('x'));
    CHECK(isspace(' '));
    CHECK(isspace('\n'));
    CHECK(tolower('Z') == 'z');
    CHECK(toupper('z') == 'Z');

    /* wide strings */
    unsigned short w[] = L"hello";
    CHECK(wcslen(w) == 5);
    CHECK(wcscmp(w, L"hello") == 0);

    /* math (minimal sanity) */
    CHECK(fabs(-1.5) == 1.5);
    CHECK(floor(3.7) == 3.0);
    CHECK(ceil(3.2) == 4.0);
    CHECK((long)(fmod(7.5, 2.5) * 10) == 0);
    CHECK(sin(0.0) == 0.0);
    CHECK((long)(cos(0.0) * 10) == 10);

    /* mbstowcs */
    unsigned short wb[8];
    mbstowcs(wb, "ab", 8);
    CHECK(wb[0] == 'a' && wb[1] == 'b' && wb[2] == 0);

    /* wcsnlen / wmemset / wmemchr */
    unsigned short w2[] = L"abcdef";
    CHECK(wcsnlen(w2, 100) == 6);
    CHECK(wcsnlen(w2, 3) == 3);
    wmemset(wb, L'Z', 3); wb[3] = 0;
    CHECK(wb[0] == L'Z' && wb[2] == L'Z');
    CHECK(wmemchr(w2, L'c', 6) == &w2[2]);

    /* strnlen / strndup */
    CHECK(strnlen("hello", 3) == 3);
    CHECK(strnlen("hi", 100) == 2);
    char *sd = strndup("abcdef", 3);
    CHECK(sd != NULL && strcmp(sd, "abc") == 0);
    free(sd);

    /* strtol/strtoul */
    CHECK(strtoull("18446744073709551615", NULL, 10) == 18446744073709551615ULL);
    CHECK(strtoll("-9223372036854775807", NULL, 10) == -9223372036854775807LL);

    /* math extras */
    CHECK(fmin(2.0, 3.0) == 2.0);
    CHECK(fmax(-1.0, -3.0) == -1.0);
    CHECK(copysign(3.0, -1.0) == -3.0);
    CHECK(trunc(3.7) == 3.0);
    CHECK(round(2.5) == 3.0);
    CHECK((long)sqrt(144.0) == 12);

    /* bsearch */
    int arr2[] = {1,3,5,7,9,11};
    int k = 7;
    int *bp2 = (int*)bsearch(&k, arr2, 6, sizeof(int), cmp_int);
    CHECK(bp2 != NULL && *bp2 == 7);

    /* memmem / strnstr */
    CHECK(memmem("hello world", 11, "world", 5) != NULL);
    CHECK(strnstr("hello world", "lo wo", 8) != NULL);
    CHECK(strnstr("hello world", "xyz", 11) == NULL);

    /* sscanf basics */
    int a = 0, b = 0;
    CHECK(sscanf("3 5", "%d %d", &a, &b) == 2);
    CHECK(a == 3 && b == 5);

    /* locale stub */
    CHECK(setlocale(LC_ALL, "C") != NULL);
    struct lconv *lc = localeconv();
    CHECK(lc != NULL && strcmp(lc->decimal_point, ".") == 0);

    /* div / ldiv */
    div_t dr = div(10, 3);
    CHECK(dr.quot == 3 && dr.rem == 1);
    ldiv_t ld = ldiv(-10L, 3L);
    CHECK(ld.quot == -3 && ld.rem == -1);

    /* itoa */
    char itbuf[32];
    itoa(255, itbuf, 16);
    CHECK(strcmp(itbuf, "ff") == 0);
    itoa(-42, itbuf, 10);
    CHECK(strcmp(itbuf, "-42") == 0);

    /* strnstr */
    CHECK(strnstr("hello world", "lo wo", 8) != NULL);

    /* bsearch */
    /* (already covered earlier; skip duplicate) */

    /* memmem / strnstr */
    CHECK(memmem("hello world", 11, "world", 5) != NULL);
    CHECK(strnstr("hello world", "xyz", 11) == NULL);

    /* sscanf basics (already checked) */

    /* locale */
    /* (already covered) */

    /* wctype */
    CHECK(iswdigit(L'5'));
    CHECK(iswalpha(L'A'));
    CHECK(iswspace(L' '));
    CHECK(towupper(L'a') == L'A');
    CHECK(towlower(L'Z') == L'z');

    /* strnlen / stpcpy / strndup / strrev / strset */
    char xbuf[16];
    strcpy(xbuf, "abcd");
    char *eos = stpcpy(xbuf, "XY");
    CHECK(eos == xbuf + 2 && *eos == '\0');
    CHECK(strcmp(xbuf,"XY") == 0);
    CHECK(strnlen(xbuf, 100) == 2);
    char *nd = strndup("hi!", 2);
    CHECK(nd && strcmp(nd, "hi") == 0);
    free(nd);
    strrev(xbuf);
    CHECK(strcmp(xbuf,"YX") == 0);
    strcpy(xbuf,"AB");
    strset(xbuf, 'Q');
    CHECK(xbuf[0]=='Q' && xbuf[1]=='Q');

    /* bstring */
    char bb[4]; bzero(bb,4);
    CHECK(bb[0]==0&&bb[3]==0);
    bcopy("ab", bb, 2);
    CHECK(bcmp(bb,"ab",2)==0);

    /* wmemcpy / wmemcmp */
    unsigned short w3[4];
    wmemcpy(w3, L"abc", 4);
    CHECK(wmemcmp(w3, L"abc",4)==0);

    /* math extras */
    CHECK(fmin(2.0,3.0)==2.0);
    CHECK(fmax(-1.0,-3.0)==-1.0);
    CHECK(copysign(3.0,-1.0)==-3.0);
    CHECK(trunc(3.7)==3.0);
    CHECK(round(2.5)==3.0);
    CHECK((long)sqrt(144.0)==12);

    /* llabs/lldiv */
    CHECK(llabs(-(long long)1<<40) == (long long)1<<40);
    lldiv_t ll = lldiv(10LL,3LL);
    CHECK(ll.quot == 3 && ll.rem == 1);

    /* buffered FILE I/O via host backend */
    const char *tp = "/tmp/akari_t1.tmp";
    unlink(tp);
    FILE *fw = fopen(tp, "wb");
    CHECK(fw != NULL);
    const char *wmsg = "Hello, Akari!\nLine2\n";
    CHECK(fwrite(wmsg,1,strlen(wmsg),fw) == strlen(wmsg));
    CHECK(ftell(fw) == (long)strlen(wmsg));
    CHECK(fputc('Z', fw) == 'Z');
    CHECK(fclose(fw) == 0);

    FILE *fr = fopen(tp, "rb");
    CHECK(fr != NULL);
    char rbuf[64]; memset(rbuf,0,sizeof(rbuf));
    size_t nr = fread(rbuf,1,sizeof(rbuf)-1,fr);
    CHECK(nr == strlen(wmsg)+1);
    CHECK(memcmp(rbuf, wmsg, strlen(wmsg)) == 0);
    CHECK(rbuf[strlen(wmsg)] == 'Z');
    CHECK(feof(fr));
    CHECK(fseek(fr, 0L, SEEK_SET) == 0);
    CHECK(ftell(fr) == 0);
    int c = fgetc(fr); CHECK(c == 'H');
    CHECK(ungetc('X', fr) == 'X');
    CHECK(fgetc(fr) == 'X');
    CHECK(fgetc(fr) == 'e');
    CHECK(fclose(fr) == 0);

    FILE *fa = fopen(tp, "ab");
    CHECK(fa != NULL);
    CHECK(fputs("appended", fa) == 0);
    CHECK(fclose(fa) == 0);
    fr = fopen(tp, "rb");
    CHECK(fr != NULL);
    memset(rbuf,0,sizeof(rbuf));
    nr = fread(rbuf,1,sizeof(rbuf)-1,fr);
    CHECK(nr == strlen(wmsg)+1+8);
    CHECK(memcmp(rbuf+strlen(wmsg)+1,"appended",8) == 0);
    fclose(fr);
    unlink(tp);

    /* wcstol / wcstoul / wcstod */
    wchar_t *we = NULL;
    CHECK(wcstol(L"  42xyz", &we, 10) == 42);
    CHECK(we && *we == L'x');
    CHECK(wcstoul(L"0xFF", &we, 16) == 255);
    CHECK(wcstod(L"3.14end", &we) > 3.13 && wcstod(L"3.14end", &we) < 3.15);

    /* fdopen and fd 0/1/2 basics */
    CHECK(fileno(stdin) == 0);
    CHECK(fileno(stdout) == 1);
    CHECK(fileno(stderr) == 2);
    CHECK(ferror(stdin)==0);
    clearerr(stdin);

    /* sprintf via snprintf path still works */
    char fbuf[64];
    int sr = sprintf(fbuf,"n=%d x=%s",42,"hi");
    CHECK(sr == 9);
    CHECK(strcmp(fbuf,"n=42 x=hi") == 0);

    /* %f/%e/%g float formatting */
    sprintf(fbuf,"%.1f",3.5);
    CHECK(strcmp(fbuf,"3.5") == 0);
    sprintf(fbuf,"%.1e",10.0);
    CHECK(fbuf[0]=='1' && fbuf[1]=='.');

    /* strcasecmp */
    CHECK(strcasecmp("Hello","hello") == 0);
    CHECK(strncasecmp("FoObar","foo",3) == 0);

    /* strlcpy/strlcat */
    char lb[8];
    CHECK(strlcpy(lb,"hi",sizeof(lb))==2 && strcmp(lb,"hi")==0);
    strcpy(lb,"ab");
    CHECK(strlcat(lb,"cd",sizeof(lb))==4 && strcmp(lb,"abcd")==0);

    /* strsep */
    char sbuf[] = "a,b,c"; char *sp2 = sbuf;
    char *t1 = strsep(&sp2,","); CHECK(t1 && !strcmp(t1,"a"));

    /* strdup */
    char *d2 = strdup("abc"); CHECK(d2 && !strcmp(d2,"abc")); free(d2);

    /* strlwr/strupr */
    char xb[] = "AbC";
    CHECK(!strcmp(strlwr(xb),"abc"));
    CHECK(!strcmp(strupr(xb),"ABC"));

    /* wcsnlen / wcsdup */
    CHECK(wcsnlen(L"abc",10)==3);
    wchar_t *wd = wcsdup(L"xy"); CHECK(wd && wd[0]==L'x'&&wd[2]==L'\0'); free(wd);

    /* wmemset / wmemchr */
    wchar_t wmb[4]; wmemset(wmb,L'z',3); wmb[3]=L'\0';
    CHECK(wmb[0]==L'z' && wmemchr(wmb,L'z',3)==wmb);

    /* wcsstr / wcscasecmp */
    CHECK(wcsstr(L"hello",L"ll")!=NULL);
    CHECK(wcscasecmp(L"AbC",L"aBc")==0);

    /* wcstok */
    wchar_t ws[] = L"a,b,c"; wchar_t *st=NULL;
    wchar_t *w1 = wcstok(ws,L",",&st);
    CHECK(w1 && w1[0]==L'a');
    (void)w1;

    /* swscanf */
    int wi=0; wchar_t ws2[16];
    swscanf(L"42 hello",L"%d %ls",&wi,ws2);
    CHECK(wi == 42);
    CHECK(ws2[0] == L'h');

    /* _itow */
    wchar_t wib[20];
    _itow(255,wib,16);
    CHECK(wib[0]==L'f' && wib[1]==L'f');

    /* fwide */
    CHECK(fwide(stdin,0)==0);

    /* tmpnam */
    char *tn = tmpnam(NULL); CHECK(tn && tn[0]);

    printf("\n%d ok, %d failed\n", ok, fail);
    return fail ? 1 : 0;
}
