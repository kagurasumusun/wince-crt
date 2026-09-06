/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stdio_stubs.c -- stdin/stdout/stderr objects and init flag.
 * Kept in its own translation unit to break circular init between
 * stdio.c and printf.c.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "file_local.h"

static int _akari_stdio_init_done = 0;
int  _akari_stdio_inited(void)      { return _akari_stdio_init_done; }
void _akari_stdio_mark_init(void)   { _akari_stdio_init_done = 1; }

static unsigned char _sin_buf[AKARI_BUFSIZ];
static unsigned char _sout_buf[AKARI_BUFSIZ];
static unsigned char _serr_buf[AKARI_BUFSIZ];

static struct _akari_FILE _akari_stdin_file  = {
    AKARI_FILE_MAGIC, 0, _AKARI_F_READ, -1,
    _sin_buf, AKARI_BUFSIZ, 0, 0, 0, 0, 0L
};
static struct _akari_FILE _akari_stdout_file = {
    AKARI_FILE_MAGIC, 1, _AKARI_F_WRITE|_AKARI_F_TTY, -1,
    _sout_buf, AKARI_BUFSIZ, 0, 0, 0, 0, 0L
};
static struct _akari_FILE _akari_stderr_file = {
    AKARI_FILE_MAGIC, 2, _AKARI_F_WRITE|_AKARI_F_TTY, -1,
    _serr_buf, 0, 0, 0, 0, 0, 0L  /* stderr is unbuffered */
};

FILE *stdin  = (FILE *)&_akari_stdin_file;
FILE *stdout = (FILE *)&_akari_stdout_file;
FILE *stderr = (FILE *)&_akari_stderr_file;
