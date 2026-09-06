# Akari CRT (明かり) — Windows CE startup glue for Clang/LLVM

Akari (明かり — "light" in Japanese) is the tiny, architecture-neutral PE
startup glue that gets a program compiled with Clang/lld for **Windows CE 4
through 6** from the PE entry point to the user's `main` / `WinMain` /
`wWinMain`, with command-line arguments parsed and C++ constructors
executed.

Akari is **NOT a C library, NOT a C++ runtime, NOT compiler-rt, NOT a
WinCE SDK, and NOT a linker script**. All of those come from other parts
of the LLVM/Clang ecosystem:

| Component | Provided by |
|-----------|-------------|
| C standard library (`malloc`, `printf`, `exit`, `errno`, `atexit`, `memcpy`, `str*`, ...) | **libc** of your choice.  On stock Windows CE images coredll.dll exports an msvcrt-compatible C surface (`malloc`/`free`/`printf`/`exit`/`strlen`/… at fixed ordinals).  Newlib or llvm-libc also work. |
| C++ runtime (exceptions, RTTI, `__cxa_atexit`, `__gxx_personality_v0`, operator new/delete, pure-virtual handler) | **libc++ / libc++abi** (or whichever C++ runtime you choose).  coredll.dll also exports operator new/delete at ordinals 1094/1095/1456/1457 (MS C++ ABI). |
| Compiler builtins (`__chkstk`, `__aeabi_idivmod`, `__udivmodsi3`, `__aeabi_memcpy`, stack-chk fail) | **compiler-rt** — clang links `libclang_rt.builtins-<arch>.a` automatically when you don't pass `-nostdlib`. |
| Linker script, PE sections, subsystem selection, .pdata/.xdata EH tables | **lld**.  Pass `-Wl,-subsystem:windowsce:<ver> -Wl,-entry:<...Startup>` on the link line.  No script required. |
| Win32 SDK headers, `coredll.lib` import library | Your WinCE SDK / platform builder output. |

What Akari **does** provide — in just **two C translation units** — is the
thin glue between the OS loader and the user's entry point:

1. **`src/crt/crt0.c`** — EXE entry points `WinMainCRTStartup`,
   `wWinMainCRTStartup`, `mainCRTStartup`, plus the `/ENTRY` aliases
   `mainACRTStartup` and `mainWCRTStartup` that Microsoft's linker
   recognises.  Each entry point:
   - calls `GetModuleHandleW(NULL)` to get the HINSTANCE;
   - calls `GetCommandLineW()` and parses it with a clean-room
     CommandLineToArgvW-compatible parser (whitespace/quotes/backslash
     2N/2N+1 escape rules) into the MSVCRT globals `__argc`, `__wargv`,
     narrow `__argv`, and `_acmdln`;
   - runs C++ constructors from `.init_array` (and legacy `.ctors` for
     toolchains that still emit it);
   - invokes the user's weak `WinMain`/`wWinMain`/`main`, with sensible
     fallbacks (WinMain chains to main if WinMain is not defined);
   - calls `exit(return_code)` from the C library, which runs
     `atexit`-registered functions (including `__cxa_atexit` C++
     destructors) before libc invokes `ExitProcess`.

2. **`src/crt/dllcrt.c`** — `_DllMainCRTStartup` for DLLs: on
   `DLL_PROCESS_ATTACH` it calls `DisableThreadLibraryCalls` and runs
   `.init_array` constructors, on `DLL_PROCESS_DETACH` it dispatches to
   the user's weak `DllMain` and then runs `.fini_array` destructors,
   and forwards thread notifications (which are normally suppressed)
   straight through.

Akari **defines** the MSVCRT data globals that coredll does NOT export
but that every Win32 C program expects to exist: `__argc`, `__argv`,
`__wargv`, `_acmdln`, `_wcmdln`, `_fmode`, `_doserrno`, `_commode`.
Functions like `exit`, `malloc`, `free`, `strlen`, `printf`,
`__cxa_atexit`, `__gxx_personality_v0`, `operator new`, `__chkstk` are
all **imported** from libc / libc++ / compiler-rt / coredll at link
time; Akari does not provide them.

---

## Supported architectures and CE versions

Akari is written to be **architecture-neutral**.  All sources use C99
plus a small set of compiler attributes; there is no inline assembly, no
endianness assumption, no pointer-width assumption, and no calling-
convention decorator that hard-codes one ISA.  In particular the
`WINAPI` macro expands to nothing on Windows CE, which matches the
platform ABI:

| Architecture | CE versions | Calling convention for coredll exports |
|--------------|-------------|----------------------------------------|
| ARM (v4, v4i, v5, v6, v7, Thumb/Thumb2) | 4.0–6.0 (and 7) | Platform default (ARM EABI APCS / AAPCS); no `__stdcall` |
| x86 (i486 and later, CEPC / emulator) | 4.0–6.0 | `__cdecl` (note: *not* `__stdcall` like desktop Win32!) |
| MIPS (MIPSII, MIPSII_FP, MIPSIV, MIPSIV_FP, MIPS16) | 4.0–6.0 | Platform default; no `__stdcall` |
| SuperH (SH3, SH4) | 4.0–6.0 (H/PC, handhelds) | Platform default; no `__stdcall` |

The `WINAPI` macro in `include/akari/compiler.h` only expands to
`__attribute__((stdcall))` when compiling for **desktop** (non-CE)
32-bit x86 Win32, which is not a supported Akari target but keeps the
header usable for host tools.  For `_WIN32_WCE` (and any non-x86 target)
it is empty.

Select a target by overriding `CROSS`:

```sh
make CROSS=armv4-wince-      # ARMv4  (Pocket PC 2003 / Windows Mobile 2003 class)
make CROSS=armv5-wince-      # ARMv5  (Windows CE 5 / Windows Mobile 5)
make CROSS=armv7-wince-      # ARMv7 Thumb2 (CE 6 / Windows Mobile 6.5)
make CROSS=i686-wince-       # x86    (CEPC / DeviceEmulator)
make CROSS=mips-wince-       # MIPS
make CROSS=sh4-wince-        # SuperH 4
```

---

## Building

Toolchain: **clang + lld + llvm-ar** from the
`kagurasumusun/llvm-project` fork (or any clang/lld build with a
`*-windows-gnu` driver for the chosen CPU).

```sh
make CROSS=armv7-wince-
```

produces:

```
build/libakari.a         both object files, for linkers that prefer an archive
build/akari_crt0.o       WinMainCRTStartup   (+ mainACRTStartup alias)
build/akari_crt0w.o      wWinMainCRTStartup  (+ mainWCRTStartup alias)
build/akari_crt0c.o      mainCRTStartup
build/akari_dllcrt.o     _DllMainCRTStartup
```

Install with:

```sh
make PREFIX=/path/to/sysroot install
```

### Typical link line (ARMv7 Thumb2, CE 6)

```sh
clang --target=thumbv7-unknown-windows-gnu \
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

For x86 CE replace `--target=thumbv7-unknown-windows-gnu` with
`--target=i686-unknown-windows-gnu`; the subsystem version 9.0 is CE 6.x
(use `4.0` for CE 4, `5.0`/`5.01`/`5.02` for CE 5.x).

Notes:

* `-lcoredll` links the OS import library; coredll.dll provides kernel,
  file, memory, and the msvcrt-compatible C exports.
* Drop `-nostdlib` so clang adds `libclang_rt.builtins-<arch>.a`
  automatically for `__chkstk`, `__aeabi_*`, etc.
* If you do not want to rely on coredll's built-in C exports, link your
  own libc (newlib / llvm-libc) instead of `-lcoredll`; Akari only
  calls `malloc`, `free`, `exit`, and a handful of Win32 entry points.

### Host-side build check

```sh
make hostcheck
```

compiles both Akari TUs warning-free with the host `gcc` (`-Wall -Wextra
-std=c99 -Wshadow -Wstrict-prototypes -Wmissing-prototypes`) and archives
them to catch syntax and portability mistakes early without a cross
toolchain. Akari does not ship runtime unit tests — the code it
contains is platform entry glue that must be validated on-target.

---

## Repository layout

```
include/akari/compiler.h   Compiler macros (NORETURN/WEAK/USED/AKARI_DLLIMPORT/WINAPI).
                           WINAPI expands to nothing on Windows CE.
include/akari/crt.h        Declares the MSVCRT data globals Akari defines.
src/crt/crt0.c             EXE entry points: WinMainCRTStartup / wWinMainCRTStartup /
                           mainCRTStartup (+ mainACRTStartup / mainWCRTStartup aliases).
src/crt/dllcrt.c           DLL entry point: _DllMainCRTStartup.
```

---

## Design notes

* **No `__stdcall` in CE exports.** This is the most common mistake
  when porting desktop Win32 startup code to Windows CE.  Desktop Win32
  uses `__stdcall` for almost every system call, decorated with an
  `@N` suffix on x86; Windows CE does not.  On CE ARM, MIPS, and SH
  there is only one calling convention.  On CE x86, coredll uses
  `__cdecl` (no `@N` suffix).  Declaring coredll imports with
  `__attribute__((stdcall))` under x86 CE causes the linker to look
  for `_GetModuleHandleW@4` and similar decorated names that do not
  exist, and even if you patched that, the callee/cleanup mismatch
  would corrupt the stack at runtime.
* **`WCHAR` is `wchar_t`, not `unsigned short`.** Clang treats
  `wchar_t` as a distinct type from `unsigned short`; if the startup
  code declared `WCHAR` as `unsigned short`, calls to coredll APIs
  that take `LPCWSTR` would compile with a type mismatch and (on
  architectures where `wchar_t` is unsigned 32-bit) crash.
* **Byte counts use `SIZE_T` (`size_t`).** Casting `sizeof(...)` to
  `unsigned` truncates on 64-bit (which we don't target) but, more
  importantly, is misleading: Win32 allocators expect
  pointer-width-typed sizes.
* **`used` attribute on entry points.** The PE entry points are not
  referenced from any other C code in the link unit; without
  `__attribute__((used))` LTO and/or `--gc-sections` could discard them.
* **Weak default `WinMain`/`DllMain`.** If a consumer defines only
  `main()`, `WinMainCRTStartup` chains into it automatically.  If a
  DLL consumer defines no `DllMain`, Akari's default returns `TRUE`.
* **`.init_array` is primary; `.ctors` is fallback.** Clang/lld uses
  `.init_array`; legacy `.ctors` is walked only if present, for
  compatibility with code built by older ARM-CE GCC ports.
* **No CommandLineToArgvW from shell32.** We ship our own parser so
  the CRT depends only on coredll.dll; shell32 is not available on
  all CE configurations (e.g. headless kernel-only images).
* **No ANSI `WinMain` on CE.** Windows CE's native `WinMain` takes
  `LPWSTR`, not `LPSTR`. Akari exposes `WinMain(HINSTANCE, HINSTANCE,
  LPWSTR, int)` natively and treats `wWinMain` as an alias, which
  matches Microsoft's CE documentation.

---

## Clean-room status

Akari is written from scratch, clean-room, under the MIT license.  **No
code from msvcrt.dll, mingw-w64, mingwrt, cegcc/mingw32ce, w32api,
newlib, glibc, dietlibc, or musl was copied, adapted, or used as a
reference during implementation.** Public information sources were
limited to:

* ISO C99 / C11 and the Itanium C++ ABI specification.
* Microsoft's public MSDN / previous-versions documentation of the
  PE/COFF format, the Windows CE Win32 API (`WinMain`,
  `GetModuleHandleW`, `GetCommandLineW`, `LocalAlloc`,
  `DisableThreadLibraryCalls`, `DllMain`), and the CE-specific notes
  on calling conventions.
* Publicly visible coredll.dll ordinal listings, used only to confirm
  which symbols are exported by the OS (and therefore should be
  imported rather than defined by Akari).

## License

MIT — see the copyright notice at the top of each source file and the
LICENSE file.

&copy; 2026 Akari CRT contributors.
