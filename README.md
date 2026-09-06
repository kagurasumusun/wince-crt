# Akari CRT (明かり) — Windows CE startup / ABI glue for Clang/LLVM

Akari is a **minimal CRT (C Runtime) glue layer** for compiling C/C++
programs with LLVM/Clang/lld and running them on **Windows CE (ARM
Thumb, PE/COFF, subsystem 9)**.

This repository is **NOT a C standard library** and **NOT a WinCE SDK**.
It supplies only the glue necessary to get a Clang-generated PE binary
from its entry point to the user's `main`/`WinMain`/`wWinMain` with
C++ static constructors run, thread-safe `errno`, atexit handlers, and
the MSVCRT-visible global symbols that compiled code expects.

You bring:

- The **C/C++ standard library** (e.g. llvm-libc, newlib, musl, or the
  msvcrt-compatible exports already present in `coredll.dll`).
- The **WinCE SDK headers / import library** (`windows.h`, `coredll.lib`,
  etc.). Those describe the Win32 surface; Akari does not.

> **Akari (明かり)** is Japanese for "light" — a small, bright glue
> layer between Clang's output and the CE kernel.

Clean-room MIT-licensed implementation; see §*Clean-room status*.

---

## What Akari provides

| Component | File | Purpose |
|-----------|------|---------|
| EXE startup | `src/crt/crt0.c` | `WinMainCRTStartup` / `wWinMainCRTStartup` / `mainCRTStartup`: TLS/errno init, `.init_array` / `.ctors` dispatch, `GetCommandLineW()` parsing → `__argc`/`__argv`/`__wargv`/`_acmdln`, call user entry point, `exit` → `ExitProcess`. |
| DLL startup | `src/crt/dllcrt.c` | `_DllMainCRTStartup`: per-process init, `DisableThreadLibraryCalls`, call constructors, dispatch to user `DllMain`. |
| atexit | `src/misc/atexit.c` | `atexit`, `_onexit`, `__cxa_atexit` (minimal), `_akari_atexit_init/_fini`. |
| errno | `src/misc/errno.c` | Thread-local errno via `TlsAlloc`/`TlsGetValue`; `_errno()` accessor matching MSVCRT. |
| exit | `src/misc/exit.c` | `exit`, `_exit`, `_Exit`, `abort` → `coredll!ExitProcess`. |
| MSVCRT globals | `src/misc/globals.c` | `__argc`, `__argv`, `__wargv`, `_acmdln`, `_fmode`, `_doserrno`, `__iob_func`, `__p___argv`, `_initterm`/`_initterm_e`, `__imp___*`. |
| C++ glue | `src/crt/crt_cpp.c` | `operator new`/`delete` (`??2`/`??3`/`??_U`/`??_V` + Itanium aliases), `__CxxFrameHandler3` stubs, `__cxa_guard_*`, `__RTDynamicCast`, `_purecall`, stack-chk guard. |
| Import thunks | `src/crt/patchables.c` | `__imp_malloc`/`__imp_free` absolute-import pointers. |
| ARM helpers | `src/compiler-rt/` | `__chkstk` (ARM stack probe), `__aeabi_idivmod` (needed by some CE targets). |

What Akari **deliberately does not provide**:

- Anything from `<stdio.h>`, `<string.h>`, `<stdlib.h>`, `<math.h>`,
  `<ctype.h>`, `<time.h>`, `<wchar.h>`, etc. That is libc's job.
- `windows.h`, `windef.h`, `winbase.h`, or any Win32 API signature
  beyond the handful of coredll entry points Akari itself calls.
- `coredll.lib` / `.def` import library.
- A linker script. PE subsystem selection is done via
  `-Wl,-subsystem:windowsce:9.0`; section placement is lld's job. A
  sample `ldscripts/arm-wince.ld` ships in `samples/` for reference.

---

## Building & linking

Toolchain: **clang + lld + llvm-ar** from LLVM, targeting
`thumbv7-unknown-windows-gnu` (the ARM Windows PE target that ships in
the `kagurasumusun/llvm-project` fork).

```sh
make CROSS=armv7-wince-
```

produces:

```
build/libakari.a
build/akari_crt0.o    # WinMainCRTStartup
build/akari_crt0w.o   # wWinMainCRTStartup
build/akari_crt0c.o   # mainCRTStartup
build/akari_dllcrt.o  # _DllMainCRTStartup
```

Install with `make PREFIX=/path/to/sysroot install`.

### Typical link line (user application)

```sh
clang --target=thumbv7-unknown-windows-gnu -fshort-wchar -mthumb \
      -nostdlib -fuse-ld=lld \
      -I<your-sdk>/include \
      -Wl,-subsystem:windowsce:9.0 \
      -Wl,-entry:WinMainCRTStartup \
      your_code.o \
      <path>/akari_crt0.o -L<path> -lakari \
      -L<your-sdk>/lib -lcoredll -lyourlibc -o your.exe
```

`-lcoredll` is the OS import library; `-lyourlibc` is whatever C library
you have chosen to target CE (coredll's built-in msvcrt-exports, a
newlib port, llvm-libc, …). Akari calls into malloc/free/LocalAlloc/
ExitProcess/GetCommandLineW/… — all resolved from those libraries at
link time.

### Host-side sanity check

```sh
make hostcheck
```

compiles every Akari translation unit with the host `gcc` (warning-free)
and archives them into `build/host/libakari.a`. This is a build-only
smoke test; Akari does not ship functional runtime tests because the
code paths it touches are inherently platform-specific (PE entry, TLS,
coredll calls).

---

## Repository layout

```
include/akari/compiler.h   Compiler macros (NORETURN/WEAK/WINAPI/...).
include/akari/crt.h        Declarations of MSVCRT-ABI symbols Akari exports.
src/crt/                   Startup + C++ ABI glue.
src/misc/                  atexit, errno, exit, MSVCRT globals.
src/compiler-rt/           __chkstk, __aeabi_idivmod (ARM).
samples/ldscripts/         Reference arm-wince.ld for lld (not required).
```

---

## Clean-room status

Akari is written from scratch. No code from mingwrt/w32api/cegcc/
newlib/glibc/musl/MSVCRT or any other C runtime was copied, adapted,
or referenced during implementation. Information sources used were
limited to:

- ISO C99/C11 and the C++ Itanium ABI specification.
- Microsoft's public ARM64/ARM exception handling documentation for
  `.pdata`/`.xdata` (https://learn.microsoft.com/cpp/build/arm-exception-handling).
- The MS-PE specification and lld's own PE/COFF handling (LLVM review
  D82883), used to confirm that .pdata is sorted automatically.
- Publicly documented coredll.dll exports (ordinal lists) sufficient to
  know which entry points to declare.

## License

MIT — see copyright notice at the top of each source file.

&copy; 2026 Akari CRT contributors.
