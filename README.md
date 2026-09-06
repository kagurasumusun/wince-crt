# Akari CRT (明かり) — Windows CE startup glue for Clang/LLVM

Akari is the **tiny PE startup glue** that gets a program compiled with
Clang/lld for Windows CE (ARM Thumb, subsystem 9) from the PE entry
point to the user's `main` / `WinMain` / `wWinMain`, with command-line
arguments parsed and C++ constructors executed.

Akari is **NOT a C library, NOT a C++ runtime, NOT a compiler-rt, NOT
a WinCE SDK, and NOT a linker script**. All of those come from other
parts of the LLVM/Clang ecosystem:

| Component | Provided by |
|-----------|-------------|
| C standard library (`malloc`, `printf`, `exit`, `errno`, `atexit`, `memcpy`, `str*`, ...) | **libc** of your choice. On stock Windows CE images, coredll.dll exports the msvcrt-compatible C functions (`malloc`/`free`/`printf`/`exit`/`strlen`/… at fixed ordinals). Newlib or llvm-libc work too. |
| C++ runtime (exceptions, RTTI, `__cxa_atexit`, `__gxx_personality_v0`/`__CxxFrameHandler3` intrinsics, operator new/delete, pure-virtual handler) | **libc++ / libc++abi** (or whatever C++ runtime you choose). coredll.dll already exports `??2@YAPAXI@Z` (operator new) and friends at ordinals 1094/1095/1456/1457. |
| Compiler builtins (`__chkstk`, `__aeabi_idivmod`, `__udivmodsi3`, `__aeabi_memcpy`, stack-chk fail) | **compiler-rt** — clang links `libclang_rt.builtins-arm.a` automatically when you do not pass `-nostdlib`. |
| Linker script, PE sections, subsystem 9, .pdata/.xdata EH tables | **lld** — pass `-Wl,-subsystem:windowsce:9.0 -Wl,-entry:WinMainCRTStartup`. No script required. |
| Win32 SDK headers, `coredll.lib` import library | Your WinCE SDK. |

What Akari **does** provide, in two translation units:

1. **`src/crt/crt0.c`** — three PE entry points
   (`WinMainCRTStartup`, `wWinMainCRTStartup`, `mainCRTStartup`).
   Each one:
   - calls `GetModuleHandleW(NULL)` to get the HINSTANCE,
   - calls `GetCommandLineW()` and parses it (CommandLineToArgvW-style
     rules: whitespace/quotes/backslash escapes) into the MSVCRT globals
     `__argc`, `__wargv`, narrow `__argv`, and `_acmdln`,
   - runs C++ constructors from `.init_array` (and legacy `.ctors`),
   - invokes the user's `WinMain`/`wWinMain`/`main` (each is a weak
     symbol so the linker picks whichever the user defined, with
     sensible fallbacks),
   - calls `exit(return_code)` from the C library, which runs
     atexit-registered functions (including C++ destructors) and
     invokes `ExitProcess`.

2. **`src/crt/dllcrt.c`** — `_DllMainCRTStartup` for DLLs:
   calls `DisableThreadLibraryCalls`, runs constructors, dispatches
   to a user-supplied weak `DllMain`.

Akari **defines** the MSVCRT data globals that coredll does not export
but every C program expects to exist:
`__argc`, `__argv`, `__wargv`, `_acmdln`, `_fmode`, `_doserrno`.
Functions like `exit`, `malloc`, `free`, `strlen`, `printf`,
`__cxa_atexit`, `__CxxFrameHandler3`, `operator new`, `__chkstk` are
all **imported** from libc / libc++ / compiler-rt / coredll at link
time; Akari does not provide them.

> **Akari (明かり)** is Japanese for "light": the smallest piece that
> lights up a Clang-compiled binary on Windows CE.

Clean-room MIT-licensed; see §*Clean-room status*.

---

## Building

Toolchain: **clang + lld + llvm-ar** targeting
`thumbv7-unknown-windows-gnu` (as shipped in the
`kagurasumusun/llvm-project` fork).

```sh
make CROSS=armv7-wince-
```

produces:

```
build/libakari.a       (only contains the two CRT object files;
                        also supplied as separate .o files below)
build/akari_crt0.o     WinMainCRTStartup
build/akari_crt0w.o    wWinMainCRTStartup
build/akari_crt0c.o    mainCRTStartup
build/akari_dllcrt.o   _DllMainCRTStartup
```

Install with:

```sh
make PREFIX=/path/to/sysroot install
```

### Typical link line

```sh
clang --target=thumbv7-unknown-windows-gnu -fshort-wchar -mthumb \
      -fuse-ld=lld \
      -I<your-sdk>/include \
      -Wl,-subsystem:windowsce:9.0 \
      -Wl,-entry:WinMainCRTStartup \
      your_code.o \
      <prefix>/lib/akari_crt0.o -L<prefix>/lib -lakari \
      -L<your-sdk>/lib -lcoredll \
      -lc++ -lc++abi -lunwind   # only if you use C++ exceptions
      -o your.exe
```

Notes:

* `-lcoredll` links the OS import library; coredll.dll provides
  kernel + file + memory + the msvcrt-compatible C exports.
* `-lclang_rt.builtins-arm` (or leave off `-nostdlib` entirely and
  let clang add compiler-rt automatically) supplies `__chkstk`,
  `__aeabi_*`, etc.
* If you do not want to rely on coredll's built-in C exports, link
  your own libc (newlib / llvm-libc) instead of `-lcoredll` for the C
  parts; Akari only calls `malloc`, `free`, `exit`, and Win32 entry
  points.

### Host-side build check

```sh
make hostcheck
```

compiles both Akari TUs warning-free with the host `gcc` and archives
them, to catch syntax/type errors without an ARM toolchain. Akari does
not ship runtime unit tests — the code it contains is platform entry
glue that must be validated on-target.

---

## Repository layout

```
include/akari/compiler.h   Compiler macros (NORETURN/WEAK/WINAPI/…).
include/akari/crt.h        Declares __argc/__argv/__wargv/_acmdln/_fmode
                           that Akari defines.
src/crt/crt0.c             WinMainCRTStartup / wWinMainCRTStartup / mainCRTStartup.
src/crt/dllcrt.c           _DllMainCRTStartup.
```

---

## Clean-room status

Akari is written from scratch. No code from msvcrt.dll, mingw-w64,
mingwrt, w32api, cegcc, newlib, glibc, dietlibc, musl, or any other
runtime was copied, adapted, or consulted during implementation.
Public information sources were limited to:

* ISO C99/C11 and the Itanium C++ ABI specification.
* Microsoft's public documentation of PE/COFF, ARM exception handling
  (`.pdata`/`.xdata`), and the coredll entry points Akari itself calls
  (`GetModuleHandleW`, `GetCommandLineW`, `LocalAlloc`,
  `DisableThreadLibraryCalls`).
* Publicly visible coredll ordinal lists, used only to confirm which
  symbols are exported by the OS (and therefore should be imported,
  not defined by Akari).

## License

MIT — see the copyright notice at the top of each source file and the
LICENSE file.

&copy; 2026 Akari CRT contributors.
