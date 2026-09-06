/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * locale.c -- stub locale support.  Windows CE ships with minimal
 * locale facilities; we provide the C-locale default and return NULL
 * for other requests.
 */
#include <locale.h>
#include <string.h>
#include <stddef.h>

static struct lconv _akari_lconv = {
    ".", "", "", "", "", ".", "", "", "", "",
    0, 0, 0, 0, 0, 0, 0, 0
};
static const char *_akari_cur_locale = "C";

char *setlocale(int category, const char *locale)
{
    (void)category;
    if (locale == NULL) return (char *)_akari_cur_locale;
    if (strcmp(locale, "") == 0 || strcmp(locale, "C") == 0) {
        _akari_cur_locale = "C";
        return (char *)"C";
    }
    return NULL;
}

struct lconv *localeconv(void) { return &_akari_lconv; }
