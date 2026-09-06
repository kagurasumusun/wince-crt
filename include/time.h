/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 */
#ifndef _AKARI_TIME_H_
#define _AKARI_TIME_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCKS_PER_SEC
#   define CLOCKS_PER_SEC 1000
#endif
typedef unsigned long clock_t;
typedef long          time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

clock_t    clock(void);
time_t     time(time_t *t);
double     difftime(time_t a, time_t b);
struct tm *localtime(const time_t *t);
struct tm *gmtime(const time_t *t);
time_t     mktime(struct tm *tm);
char      *ctime(const time_t *t);
char      *asctime(const struct tm *tm);
size_t     strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

#define CLK_TCK CLOCKS_PER_SEC

#ifdef __cplusplus
}
#endif

#endif /* _AKARI_TIME_H_ */
