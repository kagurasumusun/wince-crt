# Akari CRT (明かり) — Windows CE startup / ABI glue for Clang/LLVM

Akari is the **minimal CRT glue layer** that connects a PE/COFF binary
produced by `clang`/`lld` (targeting `thumbv7-unknown-windows-gnu`) to
the Windows CE kernel at runtime.

Akari is **NOT a C standard library** and **NOT a WinCE SDK**:

- **C standard library** — provided by **libc** (LLVM libc, newlib,
  musl, or the msvcrt-compatible exports built into `coredll.dll`).
  This includes `malloc`/`free`, all of `<stdio.h>`/`<stdlib.h>`/
  `<string.h>`/`<math.h>`/..., and the standard `exit()`/`abort()`/
  `_exit()` entry points.
- **Compiler builtins** — provided by **compiler-rt** and linked
  automatically by clang. This includes `__chkstk`, `__aeabi_idivmod`,
  `__udivmodsi4`, `__aeabi_memcpy`, and so on.
- **C++ runtime** — provided by **libc++ / libc++abi** (or another
  C++ runtime). Akari supplies the thin ABI shims (operator new/delete
  aliases, EH personality stubs, guard-variable helpers) that compiled
  C++ code references but the runtime doesn't otherwise guarantee.
- **WinCE SDK** — Windows API definitions and the `coredll.lib` import
  library are an SDK concern (`windows.h`, `windef.h`, `winbase.h`,
  …). Akari forward-declares only the handful of `coredll` entry
  points it needs itself (e.g. `GetCommandLineW`, `ExitProcess`,
  `Tls*`, `LocalAlloc`).
- **Linker script** — not required. Use
  `-Wl,-subsystem:windowsce:9.0 -Wl,-entry:WinMainCRTStartup`; lld
  takes care of section layout, `.pdata`/`.xdata` sorting, and
  generating the PE/CE header.

> **Akari (明かり)** is Japanese for "light": the smallest possible
> layer that lets C/C++ code compiled by Clang actually *start* on
> Windows CE.

Clean-room MIT-licensed; see §*Clean-room status*.

---

## What Akari provides

| Component | File | Purpose |
|-----------|------|---------|
| EXE startup | `src/crt/crt0.c` | `WinMainCRTStartup` / `wWinMainCRTStartup` / `mainCRTStartup`: initialise TLS/errno, run `.init_array` / `.ctors`, parse `GetCommandLineW()` into `__argc`/`__argv`/`__wargv`/`_acmdln`, call the user entry point, run atexit handlers, call `ExitProcess`. Uses *only* forward-declared coredll entry points. |
| DLL startup | `src/crt/dllcrt.c` | `_DllMainCRTStartup`: per-process init, `DisableThreadLibraryCalls`, run constructors, dispatch to the user-supplied weak `DllMain`. |
| atexit | `src/misc/atexit.c` | `atexit`, `_onexit`, `__cxa_atexit` (minimal); `_akari_atexit_init/_fini` for startup/shutdown use. Uses `malloc`/`free` from the consumer's libc. |
| TLS errno | `src/misc/errno.c` | Per-thread errno via `TlsAlloc`/`TlsGetValue`; exposes `_errno()` accessor matching MSVCRT's `#define errno (*_errno())` convention. |
| Internal shutdown | `src/misc/exit.c` | `_akari_cexit(int code)` — the only shutdown path the startup code uses. Runs atexit handlers and calls `ExitProcess`. **Does not** define `exit()`/`abort()`; those come from libc. |
| MSVCRT globals | `src/misc/globals.c` | `__argc`, `__argv`, `__wargv`, `_acmdln`, `_fmode`, `_doserrno`, `__iob_func`, `__p___argc`/`__p___argv`/`__p___wargv`, `_initterm`/`_initterm_e`, `__imp____argc`. |
| C++ ABI glue | `src/crt/crt_cpp.c` | `operator new`/`delete`/`new[]`/`delete[]` under both MS (`??2@YAPAXI@Z` etc.) and Itanium (`_Znwj`, `_ZdlPv`, …) manglings as weak symbols; `__cxa_guard_acquire/release/abort` for function-local statics; weak fallbacks for `__CxxFrameHandler3`, `_CxxThrowException`, `__RTDynamicCast`, `_purecall`, `__stack_chk_fail`, etc. |
| Import thunks | `src/crt/patchables.c` | Weak `__imp_malloc`/`__imp_free` absolute-pointer slots for dllimport calls. |

---

## Building

Toolchain: **clang + lld + llvm-ar** from LLVM, targeting
`thumbv7-unknown-windows-gnu` (as shipped in the
`kagurasumusun/llvm-project` fork).

```sh
make CROSS=armv7-wince-
```

produces:

```
build/libakari.a       static CRT glue library
build/akari_crt0.o     WinMainCRTStartup
build/akari_crt0w.o    wWinMainCRTStartup
build/akari_crt0c.o    mainCRTStartup
build/akari_dllcrt.o   _DllMainCRTStartup
```

Install with:

```sh
make PREFIX=/path/to/sysroot install
```

### Linking an application

```sh
clang --target=thumbv7-unknown-windows-gnu -fshort-wchar -mthumb \
      -nostdlib -fuse-ld=lld \
      -I<your-sdk>/include \
      -Wl,-subsystem:windowsce:9.0 \
      -Wl,-entry:WinMainCRTStartup \
      your_code.o \
      <prefix>/lib/akari_crt0.o -L<prefix>/lib -lakari \
      -L<your-sdk>/lib -lcoredll -lclang_rt.builtins-arm \
      -lyourlibc -lyourlibcxx -o your.exe
```

Notes:

* `-lcoredll` — OS import library from your WinCE SDK.
* `-lclang_rt.builtins-arm` (or just let clang link the builtins
  automatically when you don't pass `-nostdlib`) supplies `__chkstk`,
  `__aeabi_*`, etc.
* `-lyourlibc` / `-lyourlibcxx` — your chosen C/C++ library (coredll
  msvcrt exports, llvm-libc, newlib + libc++/libc++abi, …).

### Host-side build check

```sh
make hostcheck
```

compiles every Akari translation unit with the host `gcc`,
warning-free, and archives them into `build/host/libakari.a`. This is
a pure build smoke test; Akari does not ship runtime unit tests
because the code paths it touches are inherently platform-specific.

---

## Repository layout

```
include/akari/compiler.h   Compiler macros (NORETURN/WEAK/WINAPI/…).
include/akari/crt.h        Declarations of the MSVCRT-ABI symbols Akari exports.
src/crt/                   Startup and C++ ABI glue.
src/misc/                  atexit, TLS errno, internal shutdown, MSVCRT globals.
```

---

## Clean-room status

Akari is written from scratch. No code from msvcrt.dll, mingw-w64,
mingwrt, w32api, cegcc, newlib, glibc, dietlibc, musl, or any other
runtime was copied, adapted, or consulted during implementation.
Information sources were limited to:

* ISO C99/C11 and the Itanium C++ ABI specification.
* Microsoft's public documentation for ARM exception handling
  (`.pdata`/`.xdata`), PE/COFF, and the coredll entry points Akari
  itself calls.
* LLVM's own documentation and reviews (e.g. D82883 confirming lld
  sorts `.pdata` entries automatically).
* Publicly visible coredll ordinal lists, used only to confirm entry
  point names, never to copy code.

## License

MIT — see copyright notice at the top of each source file and the
LICENSE file.

&copy; 2026 Akari CRT contributors.
