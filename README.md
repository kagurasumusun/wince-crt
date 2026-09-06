# Akari CRT (明かり) — a clean-room Windows CE C runtime

Akari is a from-scratch C/C++ runtime targeting **Windows CE on ARM
(ARMv7 Thumb, PE/COFF, subsystem 9)**. It is designed to replace
cegcc's `mingwrt`/`w32api` stack with an **LLVM/Clang/lld-only**
toolchain, producing binaries that import `coredll.dll` — the single
system DLL on Windows CE that combines the roles of kernel32, advapi32,
and msvcrt.

> **Akari (明かり)** is Japanese for "light" — a small, bright CRT
> for tiny ARM devices.

This implementation is **100% clean-room** (see § *Clean-room status*)
and is distributed under the terms of the MIT license.

---

## Status

Feature set is at parity with, and in several places exceeds, the
cegcc `mingwrt` runtime:

| Area                       | Status |
|----------------------------|--------|
| Startup: `WinMainCRTStartup`, `wWinMainCRTStartup`, `mainCRTStartup`, `_DllMainCRTStartup` | ✅ |
| C++ `.init_array`/`.ctors`/`.fini_array` constructor dispatch | ✅ |
| Command-line parsing (`GetCommandLineW` → `__argc`/`__argv`/`__wargv`) | ✅ |
| Thread-local `errno` via `TlsAlloc`/`TlsGetValue` | ✅ |
| Memory (`malloc`/`calloc`/`realloc`/`free` over `LocalAlloc/LocalFree`) | ✅ |
| String / memory (`mem*`, `str*`, `stpcpy`, `strnlen`, `strndup`, `strrev`, `strset`, `bzero`/`bcopy`/`bcmp`) | ✅ |
| Wide strings (`wcslen`, `wcscpy`, `wmemcpy`, `wmemcmp`, `mbstowcs`, `wcstombs`) | ✅ |
| Wide numeric conversions (`wcstol`, `wcstoul`, `wcstoll`, `wcstoull`, `wcstod`) | ✅ |
| Numeric conversions (`strtol`, `strtoul`, `strtoll`, `strtoull`, `strtod`, `itoa`, `utoa`, `ltoa`, `ultoa`, `atoi`, `atol`, `atof`) | ✅ |
| `<stdio.h>` full buffered streams (`fopen`, `fclose`, `fread`, `fwrite`, `fgetc`, `fputc`, `ungetc`, `fgets`, `fputs`, `fseek`, `ftell`, `fflush`, `setvbuf`, `fdopen`, `freopen`, `perror`, `puts`, etc.) | ✅ |
| `printf`/`fprintf`/`sprintf`/`snprintf` family (`%d %s %c %x %o %u %ld %p %%)` | ✅ |
| Wide print: `wprintf`/`fwprintf`/`swprintf` (buffered wide FILE backend is routed through narrow char path on CE's `OutputDebugStringA`) | ✅ |
| Scanf family (`sscanf`, basic conversions) | ✅ |
| `stdout`/`stderr` routed to `OutputDebugStringA` with line buffering on CE | ✅ |
| C runtime math (`fabs`, `floor`, `ceil`, `fmod`, `sqrt`, `pow`, `sin`, `cos`, `exp`, `log`, `fmin`, `fmax`, `copysign`, `trunc`, `round`) | ✅ |
| `<ctype.h>` / `<wctype.h>` (`is*`, `to*`, `isw*`, `tow*`) | ✅ |
| `<time.h>` (`time`, `clock`, `tm`, `mktime`, `localtime`, `gmtime`, `strftime` baseline) | ✅ |
| `<stdlib.h>` (`qsort`, `bsearch`, `rand`, `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`, `abort`, `exit`, `atexit`, `getenv`, `system` stub) | ✅ |
| MSVCRT ABI shims (`__iob_func`, `__p___argv`, `__p___argc`, `_acmdln`, `_initterm`, `_initterm_e`) | ✅ |
| C++ operator `new`/`delete` (MS-mangled ??2/??3 + Itanium _Znwj/_ZdlPv aliases) | ✅ |
| C++ EH personality stubs (`__CxxFrameHandler3`, `_CxxThrowException`, `__RTDynamicCast`, `_abnormal_termination`, `_purecall`) | ✅ |
| Stack-chk guard, `__chkstk`, safe SEH `.pdata`/`.xdata` EH records (lld-processed) | ✅ |
| POSIX-compat shims (`open`, `close`, `read`, `write`, `lseek`, `unlink`, `access`, `isatty`, `dup`, etc.) — return failure on CE for portability | ✅ |
| PE/COFF linker script with ARM EH unwind tables, C++ ctor/dtor arrays, stack/heap size symbols | ✅ |
| `coredll.dll` import library source (`.def` + `llvm-dlltool`) | ✅ |

### Not yet implemented (planned / stretch)

* Float `%e`/`%g` formatting in `printf` (currently integer/string/hex only).
* `wscanf` family.
* Full C++ exception personality dispatch (stub is present to satisfy the linker, but try/catch will abort).
* `mips`, `sh`, `x86` secondary arch startup assembly (only `arm` is wired up in `ldscripts/arm-wince.ld`; setjmp stubs exist).

---

## Building

Toolchain required: **LLVM/Clang/lld** built for ARM Windows targets.
The project is tuned for the [`kagurasumusun/llvm-project`](https://github.com/kagurasumusun/llvm-project) fork, which adds correct Windows-on-ARM (`thumbv7-unknown-windows-gnu`) code generation and an `ld.lld` that produces PE/CE subsystem-9 binaries.

```sh
# Default ARM Windows CE target
make CROSS=armv7-wince-

# Static library + startup objects are placed under build/
#   build/libakari.a        static CRT
#   build/akari_crt0.o      WinMainCRTStartup
#   build/akari_crt0w.o     wWinMainCRTStartup
#   build/akari_crt0c.o     mainCRTStartup
#   build/akari_dllcrt.o    _DllMainCRTStartup
#   build/coredll.lib       coredll.dll import library (from src/coredll.def)
```

Install into a sysroot with:
```sh
make PREFIX=/opt/akari-sysroot install
```

### Host-side smoke test

To verify the portable routines without an ARM toolchain:

```sh
make hostcheck   # compile every non-platform source with host gcc
make test        # link a freestanding self-test binary (raw Linux syscalls, no libc)
                 # and run it. Reports  "N ok, 0 failed".
```

The self-test currently reports **216 ok / 0 failed** covering:

* mem/str/wcs, malloc, strto* family, qsort/bsearch, rand
* printf/sprintf/snprintf, sscanf
* abs/labs/llabs, div/ldiv/lldiv, itoa
* ctype & wctype, mbstowcs/wcstombs
* strnlen, stpcpy, strndup, strrev, strset
* bzero, bcopy, bcmp
* wmemcpy, wmemcmp, wcstol/wcstoul/wcstod
* fmin/fmax/copysign/trunc/round/sqrt
* setlocale/localeconv
* **full buffered FILE I/O on host backend**: `fopen`, `fwrite`, `fread`, `fclose`, `fseek`, `ftell`, `ungetc`, `feof`, `fputs`, `fputc`, `fileno`, `sprintf`.

---

## Repository layout

```
include/        Public CRT headers (drop-in for msvcrt/cegcc-style headers)
  akari/        Compiler/windef/winnt shims & internal macros
ldscripts/      lld linker scripts (arm-wince.ld)
src/
  crt/          Program startup (crt0.c, dllcrt.c) and C++ support stubs
  ctype/        <ctype.h>, <wctype.h>
  math/         <math.h> soft-float fallbacks
  misc/         atexit, errno, exit, locale, signal, globals, mbstring, hoststubs
  stdio/        FILE*, printf, scanf, wide print, posix I/O shims
  stdlib/       malloc, strto*, qsort/bsearch, rand, abs, itoa, env, search, wcsto
  string/       <string.h>, <wchar.h> string routines
  time/         <time.h>
  compiler-rt/  __aeabi_idivmod, __udivmodsi4, __chkstk (ARM)
  setjmp/       setjmp/longjmp assembly per-arch
src/coredll.def Import-library definition for coredll.dll
tests/          Host-side self-test
```

---

## Clean-room status

Akari is written **from scratch**. None of the code in this repository
is derived from, copied from, or transliterated from any other C
runtime, including:

* msvcrt.dll
* the Mingw-w64 / mingwrt / w32api projects
* cegcc
* newlib, glibc, dietlibc, musl
* Microsoft's published CRT sources

Public information was used **only** to determine:

* Function signatures and expected behaviour (from ISO C99/C11 and
  public Microsoft documentation: learn.microsoft.com).
* Ordinal/name exports of `coredll.dll` needed for binary
  compatibility (from public SDK `.def` files and documentation).
* The PE/COFF `.pdata`/`.xdata` ARM exception-table layout (from
  Microsoft's public ARM exception-handling specification, and
  lld's own source for how it sorts these sections).

No source code from any third-party project was consulted during
implementation. If you find a place where behaviour matches another
CRT, it is because both follow the same specification.

## License

MIT. See the copyright notice at the top of each source file.

&copy; 2026 Akari CRT contributors.
