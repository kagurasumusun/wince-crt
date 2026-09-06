/*
 * Akari C Runtime for Windows CE
 * Copyright (c) 2026 Akari CRT contributors
 * SPDX-License-Identifier: MIT
 *
 * stdio.c -- FILE streams, fopen/fclose/fread/fwrite/fflush/fgetc/fputc.
 *
 * Two backends:
 *   - Windows CE: Win32 CreateFileW/ReadFile/WriteFile/CloseHandle plus
 *                OutputDebugStringA for stdout/stderr (CE has no console).
 *   - _DEBUG_HOSTCHECK_: raw POSIX write/read/close for host self-test.
 */
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <wchar.h>
#include "printf_core.h"
#include "file_local.h"
#include <akari/compiler.h>

int  _akari_stdio_inited(void);
void _akari_stdio_mark_init(void);

void _akari_stdio_init(void)
{
    if (_akari_stdio_inited()) return;
    _akari_stdio_mark_init();
}

static int _flush_buf_locked(struct _akari_FILE *f);

/* ---- Mode parser (shared) ---- */
static int _mode_to_flags(const char *mode, int *flags_out)
{
    int f = 0;
    if (!mode || !*mode) return -1;
    if      (*mode == 'r') f |= _AKARI_F_READ;
    else if (*mode == 'w') f |= _AKARI_F_WRITE;
    else if (*mode == 'a') f |= _AKARI_F_WRITE | _AKARI_F_APPEND;
    else return -1;
    mode++;
    while (*mode) {
        switch (*mode) {
            case '+': f |= _AKARI_F_RDWR; break;
            case 'b': f |= _AKARI_F_BIN;  break;
            default: break;
        }
        mode++;
    }
    *flags_out = f;
    return 0;
}

/* --------------------------- Platform backends -------------------- */

#ifdef _DEBUG_HOSTCHECK_

extern long _akari_host_write(int fd, const void *buf, unsigned long count);
extern long _akari_host_read(int fd, void *buf, unsigned long count);
extern long _akari_host_open(const char *p, int flags, int mode);
extern long _akari_host_close(int fd);
extern long _akari_host_lseek(int fd, long off, int whence);
extern long _akari_host_unlink(const char *p);

#define _HOST_O_RDONLY 0
#define _HOST_O_WRONLY 1
#define _HOST_O_RDWR   2
#define _HOST_O_CREAT  0x40
#define _HOST_O_TRUNC  0x200
#define _HOST_O_APPEND 0x400

static int _flags_to_host(int f) {
    int acc = (f & _AKARI_F_RDWR) ? _HOST_O_RDWR :
              (f & _AKARI_F_READ) ? _HOST_O_RDONLY : _HOST_O_WRONLY;
    int extra = 0;
    if (f & _AKARI_F_APPEND) extra |= _HOST_O_APPEND|_HOST_O_CREAT;
    else if (f & _AKARI_F_WRITE) extra |= _HOST_O_CREAT|_HOST_O_TRUNC;
    return acc|extra;
}

static intptr_t _ce_open(const char *p, int flags) {
    long h = _akari_host_open(p, _flags_to_host(flags), 0644);
    return (h < 0) ? -1 : (intptr_t)h;
}
static intptr_t _ce_open_w(const wchar_t *p, int flags) { (void)p;(void)flags; return -1; }
static int _ce_write(intptr_t h, const void *b, size_t n) {
    return (int)_akari_host_write((int)h, b, (unsigned long)n);
}
static int _ce_read(intptr_t h, void *b, size_t n) {
    return (int)_akari_host_read((int)h, b, (unsigned long)n);
}
static int _ce_close(intptr_t h) {
    if (h == 0 || h == 1 || h == 2) return 0;
    return (int)_akari_host_close((int)h);
}
static long _ce_seek(intptr_t h, long o, int w) {
    return _akari_host_lseek((int)h, o, w);
}

#else

#  include <akari/windef.h>
#  include <akari/winnt.h>

static DWORD _acc_from_flags(int f) {
    DWORD acc = 0;
    if (f & _AKARI_F_READ) acc |= GENERIC_READ;
    if (f & _AKARI_F_WRITE) acc |= GENERIC_WRITE;
    return acc;
}
static DWORD _disp_from_flags(int f) {
    if (f & _AKARI_F_APPEND) return OPEN_ALWAYS;
    if (f & _AKARI_F_WRITE) return CREATE_ALWAYS;
    return OPEN_EXISTING;
}

static intptr_t _ce_open(const char *p, int flags) {
    HANDLE h = CreateFileA((LPSTR)p, _acc_from_flags(flags),
                           FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                           _disp_from_flags(flags), FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (flags & _AKARI_F_APPEND) SetFilePointer(h, 0, NULL, FILE_END);
    return (intptr_t)h;
}
static intptr_t _ce_open_w(const wchar_t *p, int flags) {
    HANDLE h = CreateFileW((LPWSTR)p, _acc_from_flags(flags),
                           FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                           _disp_from_flags(flags), FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (flags & _AKARI_F_APPEND) SetFilePointer(h, 0, NULL, FILE_END);
    return (intptr_t)h;
}

static int _dbg_line_buf_full;
static char _dbg_line[512];

static void _dbg_flush_line(void) {
    if (_dbg_line_buf_full > 0) {
        _dbg_line[_dbg_line_buf_full] = '\0';
        OutputDebugStringA(_dbg_line);
        _dbg_line_buf_full = 0;
    }
}
static void _dbg_write(const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char c = b[i];
        if (c == '\n' || _dbg_line_buf_full >= (int)sizeof(_dbg_line)-1) _dbg_flush_line();
        if (c != '\n') _dbg_line[_dbg_line_buf_full++] = c;
    }
}

static int _ce_write(intptr_t h, const void *b, size_t n) {
    if (h == 1 || h == 2) { _dbg_write((const char*)b, n); return (int)n; }
    DWORD w = 0;
    if (!WriteFile((HANDLE)h, b, (DWORD)n, &w, NULL)) return -1;
    return (int)w;
}
static int _ce_read(intptr_t h, void *b, size_t n) {
    DWORD r = 0;
    if (!ReadFile((HANDLE)h, b, (DWORD)n, &r, NULL)) return -1;
    return (int)r;
}
static int _ce_close(intptr_t h) {
    if (h == 0 || h == 1 || h == 2) { _dbg_flush_line(); return 0; }
    _dbg_flush_line();
    return CloseHandle((HANDLE)h) ? 0 : EOF;
}
static long _ce_seek(intptr_t h, long o, int w) {
    DWORD m = (w == SEEK_SET) ? FILE_BEGIN : (w == SEEK_CUR) ? FILE_CURRENT : FILE_END;
    DWORD r = SetFilePointer((HANDLE)h, o, NULL, m);
    return (r == INVALID_SET_FILE_POINTER) ? -1L : (long)r;
}

void _akari_stdio_fini_host(void) { _dbg_flush_line(); }

#endif

/* ------------------------- Generic FILE ops ------------------------ */

static int _flush_buf_locked(struct _akari_FILE *f)
{
    if ((f->flags & _AKARI_F_WRITE) && f->buf_len > 0) {
        int n = _ce_write(f->fd, f->buf, (size_t)f->buf_len);
        if (n < 0) { f->err = 1; f->flags |= _AKARI_F_ERRSEEN; return EOF; }
        f->offset += n;
        f->buf_len = 0; f->buf_pos = 0;
    }
    if (f->flags & _AKARI_F_READ) {
        /* discard unread buffered bytes: rewind fd by (buf_len - buf_pos) */
        int unread = f->buf_len - f->buf_pos;
        if (unread > 0 && f->fd > 2) _ce_seek(f->fd, -unread, 1/*SEEK_CUR*/);
        f->buf_len = 0; f->buf_pos = 0;
    }
    return 0;
}

int fflush(FILE *stream)
{
    if (stream == NULL) {
        struct _akari_FILE *all[] = {
            (struct _akari_FILE*)stdin, (struct _akari_FILE*)stdout, (struct _akari_FILE*)stderr
        };
        for (int i = 0; i < 3; i++)
            if (all[i]->flags & _AKARI_F_WRITE) _flush_buf_locked(all[i]);
#ifndef _DEBUG_HOSTCHECK_
        _akari_stdio_fini_host();
#endif
        return 0;
    }
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (f->magic != AKARI_FILE_MAGIC) { f->err=1; return EOF; }
    return _flush_buf_locked(f);
}

static int _fill_buf(struct _akari_FILE *f)
{
    if (!(f->flags & _AKARI_F_READ)) { f->err=1; return EOF; }
    if (!f->buf || f->buf_size == 0) { f->flags |= _AKARI_F_EOFSEEN; f->eof=1; return EOF; }
    int n = _ce_read(f->fd, f->buf, (size_t)f->buf_size);
    if (n <= 0) { f->flags |= _AKARI_F_EOFSEEN; f->eof = 1; return EOF; }
    f->offset += n;
    f->buf_len = n; f->buf_pos = 0; f->eof = 0;
    return (unsigned char)f->buf[0];
}

int fgetc(FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || f->magic != AKARI_FILE_MAGIC) return EOF;
    if (f->ungetc_ch >= 0) { int c = f->ungetc_ch; f->ungetc_ch = -1; return c; }
    if (f->buf_pos < f->buf_len) return (unsigned char)f->buf[f->buf_pos++];
    if (_fill_buf(f) == EOF) return EOF;
    return (unsigned char)f->buf[f->buf_pos++];
}

int ungetc(int c, FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || f->magic != AKARI_FILE_MAGIC || c == EOF) return EOF;
    f->ungetc_ch = (unsigned char)c;
    f->eof = 0; f->flags &= ~_AKARI_F_EOFSEEN;
    return (unsigned char)c;
}

int fputc(int c, FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || f->magic != AKARI_FILE_MAGIC) return EOF;
    unsigned char ch = (unsigned char)c;
    if (!(f->flags & _AKARI_F_WRITE)) { f->err=1; return EOF; }
    if (!f->buf || f->buf_size == 0) {
        if (_ce_write(f->fd, &ch, 1) != 1) { f->err=1; return EOF; }
        return ch;
    }
    if (f->buf_len >= f->buf_size && _flush_buf_locked(f) == EOF) return EOF;
    f->buf[f->buf_len++] = ch;
    if ((f->flags & _AKARI_F_TTY) && ch == '\n')
        if (_flush_buf_locked(f) == EOF) return EOF;
    return ch;
}

int fputs(const char *s, FILE *stream)
{ while (*s) if (fputc((unsigned char)*s++, stream) == EOF) return EOF; return 0; }

int putchar(int c) { return fputc(c, stdout); }

int puts(const char *s)
{
    while (*s) if (fputc((unsigned char)*s++, stdout) == EOF) return EOF;
    return fputc('\n', stdout) == EOF ? EOF : 0;
}

char *fgets(char *s, int size, FILE *stream)
{
    if (!s || size <= 0) return NULL;
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);
        if (c == EOF) { if (i == 0) return NULL; break; }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = '\0';
    return s;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    unsigned char *p = (unsigned char*)ptr;
    size_t got = 0;
    while (got < total && f->buf_pos < f->buf_len)
        p[got++] = f->buf[f->buf_pos++];
    while (got < total) {
        int n = _ce_read(f->fd, p+got, total-got);
        if (n <= 0) { f->eof=1; f->flags|=_AKARI_F_EOFSEEN; break; }
        f->offset += n;
        got += (size_t)n;
    }
    return got / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    const unsigned char *p = (const unsigned char*)ptr;
    if (!f->buf || f->buf_size == 0 || total >= (size_t)f->buf_size) {
        _flush_buf_locked(f);
        int n = _ce_write(f->fd, p, total);
        if (n < 0) { f->err=1; return 0; }
        f->offset += n;
        return (size_t)n / size;
    }
    size_t off = 0;
    while (off < total) {
        size_t space = (size_t)(f->buf_size - f->buf_len);
        size_t chunk = total-off < space ? total-off : space;
        memcpy(f->buf+f->buf_len, p+off, chunk);
        f->buf_len += (int)chunk; off += chunk;
        if (f->buf_len >= f->buf_size && _flush_buf_locked(f) == EOF)
            return off / size;
    }
    return nmemb;
}

int fseek(FILE *f, long offset, int whence)
{
    struct _akari_FILE *af = (struct _akari_FILE*)f;
    if (!af || af->magic != AKARI_FILE_MAGIC) return -1;
    _flush_buf_locked(af);
    af->ungetc_ch = -1; af->eof=0; af->flags &= ~(_AKARI_F_EOFSEEN|_AKARI_F_ERRSEEN);
    long r = _ce_seek(af->fd, offset, whence);
    if (r < 0) { af->err=1; return -1; }
    af->offset = r;
    return 0;
}
long ftell(FILE *f) {
    struct _akari_FILE *a=(struct _akari_FILE*)f;
    if (!a) return -1;
    long pos = a->offset;
    if (a->flags & _AKARI_F_READ) {
        pos -= (a->buf_len - a->buf_pos);
        if (a->ungetc_ch >= 0) pos--;
    } else if (a->flags & _AKARI_F_WRITE) {
        pos += a->buf_len;
    }
    return pos;
}
static int _feof_impl(FILE *f)   { struct _akari_FILE *a=(struct _akari_FILE*)f; return a?!!(a->flags&_AKARI_F_EOFSEEN):0; }
static int _ferror_impl(FILE *f) { struct _akari_FILE *a=(struct _akari_FILE*)f; return a?!!(a->flags&_AKARI_F_ERRSEEN):0; }
static void _clearerr_impl(FILE *f){ struct _akari_FILE *a=(struct _akari_FILE*)f; if(a){a->flags&=~(_AKARI_F_EOFSEEN|_AKARI_F_ERRSEEN);a->eof=0;a->err=0;} }
void rewind(FILE *f) { if(f) { fseek(f,0L,SEEK_SET); _clearerr_impl(f); } }
int feof(FILE *f)   { return _feof_impl(f); }
int ferror(FILE *f) { return _ferror_impl(f); }
void clearerr(FILE *f){ _clearerr_impl(f); }
int fileno(FILE *f) { struct _akari_FILE *a=(struct _akari_FILE*)f; return a?a->fd:-1; }
int _fileno(FILE *f){return fileno(f);}
int fclose(FILE *stream)
{
    struct _akari_FILE *f = (struct _akari_FILE*)stream;
    if (!f || f->magic != AKARI_FILE_MAGIC) return EOF;
    _flush_buf_locked(f);
    int r = _ce_close(f->fd);
    if (f->buf && !(f->flags & _AKARI_F_STATIC)) { free(f->buf); f->buf = NULL; }
    if (!(f->flags & _AKARI_F_STATIC)) free(f);
    return r;
}

FILE *fdopen(int fd, const char *mode)
{
    int flags; if (_mode_to_flags(mode, &flags) < 0) return NULL;
    struct _akari_FILE *f = (struct _akari_FILE*)malloc(sizeof(*f));
    if (!f) return NULL;
    memset(f,0,sizeof(*f));
    f->magic=AKARI_FILE_MAGIC; f->fd=fd; f->flags=flags;
    f->buf=(unsigned char*)malloc(AKARI_BUFSIZ); f->buf_size=AKARI_BUFSIZ;
    f->ungetc_ch=-1;
    return (FILE*)f;
}

FILE *fopen(const char *path, const char *mode)
{
    int flags; if (_mode_to_flags(mode,&flags) < 0) { errno=EINVAL; return NULL; }
    intptr_t h = _ce_open(path, flags);
    if (h == (intptr_t)-1) { errno=ENOENT; return NULL; }
    struct _akari_FILE *f = (struct _akari_FILE*)malloc(sizeof(*f));
    if (!f) { _ce_close((int)h); return NULL; }
    memset(f,0,sizeof(*f));
    f->magic=AKARI_FILE_MAGIC; f->fd=(int)h; f->flags=flags;
    f->buf=(unsigned char*)malloc(AKARI_BUFSIZ); f->buf_size=AKARI_BUFSIZ;
    f->ungetc_ch=-1;
    return (FILE*)f;
}

FILE *fopen_w(const wchar_t *path, const wchar_t *mode) {
    (void)path;(void)mode; return NULL;
}
FILE *freopen(const char *path, const char *mode, FILE *stream)
{ if(stream) fclose(stream); return fopen(path, mode); }

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
    (void)mode;
    struct _akari_FILE *f=(struct _akari_FILE*)stream; if(!f)return -1;
    if (f->buf && !(f->flags&_AKARI_F_STATIC)) free(f->buf);
    f->buf = size ? (buf?(unsigned char*)buf:(unsigned char*)malloc(size)) : NULL;
    f->buf_size = (int)size; f->buf_len=0; f->buf_pos=0;
    return 0;
}
void setbuf(FILE *stream, char *buf) { setvbuf(stream, buf, _IOFBF, BUFSIZ); }
int  setbuffer(FILE *stream, char *buf, size_t size) { return setvbuf(stream, buf, _IOFBF, size); }
int  setlinebuf(FILE *stream) { return setvbuf(stream, NULL, _IOLBF, BUFSIZ); }

void _akari_stdio_fini(void) { fflush(stdout); fflush(stderr); }

/* printf entry points route through _akari_format_core with per-stream putc/puts */
static int _sf_putc(void *ctx, int c) { return fputc(c,(FILE*)ctx); }
static int _sf_puts(void *ctx, const char *s, size_t n) {
    size_t w = fwrite(s,1,n,(FILE*)ctx);
    return w == n ? (int)n : -1;
}

int vfprintf(FILE *s, const char *fmt, va_list ap)
{ _akari_stdio_init(); return _akari_format_core(s,_sf_putc,_sf_puts,fmt,ap); }
int vprintf(const char *fmt, va_list ap) { return vfprintf(stdout,fmt,ap); }
int printf(const char *fmt,...)
    { va_list a;int n;va_start(a,fmt);n=vprintf(fmt,a);va_end(a);return n;}
int fprintf(FILE *s,const char *fmt,...)
    { va_list a;int n;va_start(a,fmt);n=vfprintf(s,fmt,a);va_end(a);return n;}

void _akari_perror(const char *s) {
    int e=errno; fprintf(stderr,"%s: %s\n",s?s:"",strerror(e));
}
void perror(const char *s) { _akari_perror(s); }

int  getc(FILE *f){return fgetc(f);}
int  putc(int c,FILE *f){return fputc(c,f);}
int  getchar(void){return fgetc(stdin);}
int  getw(FILE *f){(void)f;return EOF;}
int  putw(int w,FILE *f){(void)w;(void)f;return EOF;}
char *gets(char *s){(void)s;return NULL;}
int  _getw(FILE *f){return getw(f);}
int  _putw(int w,FILE *f){return putw(w,f);}
int  _pclose(FILE *f){(void)f;return -1;}
FILE *_popen(const char *c,const char *m){(void)c;(void)m;return NULL;}
int  _rmtmp(void){return 0;}
FILE *_fdopen(int fd,const char *m){return fdopen(fd,m);}
FILE *_wfdopen(int fd,const wchar_t *m){(void)m;return fdopen(fd,"r+");}
int  _fcloseall(void){return 0;}
int  _flushall(void){fflush(NULL);return 0;}
int  _fgetchar(void){return fgetc(stdin);}
int  _fputchar(int c){return fputc(c,stdout);}
FILE *_fsopen(const char *p,const char *m,int sh){(void)p;(void)m;(void)sh;return NULL;}
int  fgetchar(void){return _fgetchar();}
int  fputchar(int c){return _fputchar(c);}

/* --- Orientation: narrow-only; fwide(0) returns 0; non-zero mode is ignored. */
int fwide(FILE *f, int mode) { (void)f; (void)mode; return 0; /* byte-oriented */ }

/* --- Wide FILE open --- */
FILE *_wfopen(const wchar_t *path, const wchar_t *mode)
{
    char npath[260], nmode[16]; int i;
    for (i = 0; i < (int)sizeof(npath)-1 && path[i]; i++)
        npath[i] = (path[i] < 128) ? (char)path[i] : '?';
    npath[i] = 0;
    for (i = 0; i < (int)sizeof(nmode)-1 && mode[i]; i++)
        nmode[i] = (mode[i] < 128) ? (char)mode[i] : '?';
    nmode[i] = 0;
    return fopen(npath, nmode);
}
FILE *_wfreopen(const wchar_t *p, const wchar_t *m, FILE *f) { (void)p;(void)m; if(f)fclose(f); return NULL; }

/* --- Tmp / tempname stubs (CE has no standard TEMP dir) --- */
char *tmpnam(char *s) {
    static char buf[L_tmpnam];
    static int seq = 0;
    char *out = s ? s : buf;
    int n = __builtin_sprintf(out, "\\akari%03x.tmp", (unsigned)(seq++ & 0xfff)); (void)n;
    return out;
}
char *tempnam(const char *dir, const char *pfx) {
    (void)dir; (void)pfx; return NULL;
}
FILE *tmpfile(void) { return NULL; }

/* --- remove/rename via coredll --- */
int _unlink(const char *p);
int remove(const char *p) { return _unlink(p); }
int rename(const char *a, const char *b);
int rename(const char *a, const char *b) {
    (void)a;(void)b; return -1; /* CE MoveFile not in our minimal import set; stub */
}

/* --- ungetc for wide (stub returning WEOF) --- */
wint_t ungetwc(wint_t c, FILE *f) { (void)c; (void)f; return WEOF; }
wint_t fgetwc(FILE *f) { int c = fgetc(f); return c == EOF ? WEOF : (wint_t)(unsigned char)c; }
wint_t fputwc(wchar_t c, FILE *f) { char ch = (char)(unsigned short)c; return fputc(ch,f)==ch ? (wint_t)c : WEOF; }
wchar_t *fgetws(wchar_t *buf, int n, FILE *f) {
    int i = 0;
    if (!buf || n <= 0) return NULL;
    while (i < n-1) {
        int c = fgetc(f);
        if (c == EOF) { if (i == 0) return NULL; break; }
        buf[i++] = (wchar_t)(unsigned char)c;
        if (c == '\n') break;
    }
    buf[i] = L'\0';
    return buf;
}
int fputws(const wchar_t *s, FILE *f) {
    while (*s) { wchar_t c = *s++; if (fputc((int)(unsigned char)c,f) == EOF) return -1; }
    return 0;
}
wint_t getwchar(void) { return fgetwc(stdin); }
wint_t putwchar(wchar_t c) { return fputwc(c, stdout); }

/* --- flockfile/funlockfile (single-threaded: no-ops) --- */
void flockfile(FILE *f)   { (void)f; }
void funlockfile(FILE *f) { (void)f; }
int  ftrylockfile(FILE *f){ (void)f; return 0; }

/* --- clearerr unlocked variants --- */
void clearerr_unlocked(FILE *f){ clearerr(f); }
int feof_unlocked(FILE *f)     { return feof(f); }
int ferror_unlocked(FILE *f)   { return ferror(f); }
int fileno_unlocked(FILE *f)   { return fileno(f); }
int fgetc_unlocked(FILE *f)    { return fgetc(f); }
int fputc_unlocked(int c,FILE*f){return fputc(c,f);}

/* --- _getmaxstdio / _setmaxstdio --- */
int _getmaxstdio(void) { return 512; }
int _setmaxstdio(int n) { (void)n; return 512; }

/* MS underscore aliases for printf family */
int _printf(const char *f, ...){ va_list a; int n; va_start(a,f); n=vprintf(f,a); va_end(a); return n; }
int _fprintf(FILE *s, const char *f, ...){ va_list a; int n; va_start(a,f); n=vfprintf(s,f,a); va_end(a); return n; }
int _sprintf(char *str, const char *f, ...){ va_list a; int n; va_start(a,f); n=vsprintf(str,f,a); va_end(a); return n; }
int _snprintf(char *str, size_t sz, const char *f, ...){ va_list a; int n; va_start(a,f); n=vsnprintf(str,sz,f,a); va_end(a); return n; }
int _vprintf(const char *f, va_list a){ return vprintf(f,a); }
int _vfprintf(FILE *s, const char *f, va_list a){ return vfprintf(s,f,a); }
int _vsprintf(char *str, const char *f, va_list a){ return vsprintf(str,f,a); }
int _vsnprintf(char *str, size_t sz, const char *f, va_list a){ return vsnprintf(str,sz,f,a); }
int _scprintf(const char *f, ...){ va_list a; va_start(a,f); int n=vsnprintf(NULL,0,f,a); va_end(a); return n; }
int _vscprintf(const char *f, va_list a){ return vsnprintf(NULL,0,f,a); }

/* _fileno already in earlier block; _fcloseall _flushall etc already there */
