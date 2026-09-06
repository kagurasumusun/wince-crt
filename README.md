# Akari CRT (明かり) — Windows CE startup glue for Clang/LLVM

Akari (明かり — "light" in Japanese) is the tiny, architecture-neutral PE
startup glue that gets a program compiled with Clang/lld for **Windows CE 4
through 6** from the PE entry point to the user's `main` / `WinMain` /
`wWinMain`, with command-line arguments parsed, C++ constructors executed,
and shutdown properly delegated to the C library.

Akari is **NOT a C library, NOT a C++ runtime, NOT compiler-rt, NOT a
WinCE SDK, and NOT a linker script**. All of those come from other parts
of the LLVM/Clang/coredll ecosystem (see the responsibility table below).

---

## Responsibility table

| Concern | Provider | Akari ships it? |
|---|---|---|
| PE/COFF EXE entry points (`WinMainCRTStartup`, `wWinMainCRTStartup`, `mainCRTStartup`, `mainWCRTStartup`, `mainACRTStartup`) | **Akari** (`akari_crt0.o`) | ✅ |
| PE/COFF DLL entry point (`_DllMainCRTStartup`) | **Akari** (`akari_dllcrt.o`) | ✅ |
| `GetCommandLineW()` parse into `__argc`/`__wargv` (CommandLineToArgvW rules: quotes, backslash 2N/2N+1, `""`-in-quotes → literal `"`) | **Akari** (clean-room parser, only depends on coredll `LocalAlloc/LocalFree`) | ✅ |
| `WinMain`/`wWinMain` `lpCmdLine` pointer into raw command-line tail (after argv[0], preserving leading whitespace — the Win32 convention, NOT a copy of argv[1]) | **Akari** (`_wcmdtail`) | ✅ |
| Narrow `__argv`/`_acmdln` synthesis: `WideCharToMultiByte(CP_ACP)` when available, lossy ASCII fallback otherwise | **Akari** | ✅ |
| `.init_array` constructors (forward order), `.fini_array` destructors (reverse order), legacy `.ctors` (reverse-order fallback) | **Akari** | ✅ |
| MSVCRT data globals: `__argc`, `__argv`, `__wargv`, `_acmdln`, `_wcmdln`, `_wcmdtail`, `_fmode`, `_doserrno`, `_commode` | **Akari** (coredll does NOT export these data objects) | ✅ |
| Weak default `WinMain` → `main`, weak default `DllMain` → TRUE | **Akari** | ✅ |
| Shutdown handoff: call libc `exit(rc)` so that `atexit` handlers, `__cxa_atexit`-registered C++ destructors (via libc calling `__cxa_finalize`), stdio flushes, and the final `ExitProcess` all run in the right order | **Akari** (just the call; the sequencing is libc's responsibility) | ✅ stub |
| C library: `malloc`/`free`/`printf`/`exit`/`atexit`/`abort`/`strlen`/`memcpy`/`setjmp`/`longjmp`/errno accessors | **libc** (coredll msvcrt, newlib, llvm-libc) | ❌ |
| `__stack_chk_guard` / `__stack_chk_fail` (stack protector) | **libc** (or compiler-rt) — Akari compiles its own startup files with `-fno-stack-protector` because the guard is not yet valid when the entry point runs | ❌ |
| `__cxa_atexit`, `__cxa_finalize`, `__cxa_pure_virtual`, operator `new`/`delete`, EH personality (`__gxx_personality_v0`), RTTI | **libc++ / libc++abi** (coredll also exports MS-C++ new/delete @1094/1095/1456/1457) | ❌ |
| Exception unwinding (ARM EHABI `.pdata`/`.xdata` walking, Itanium LSDA) | **llvm-libunwind** / libc++abi's built-in unwinder | ❌ |
| Compiler builtins: `__chkstk`, `__aeabi_*` division/memory helpers, `__udivmodsi3`, `__floatdidf`, ... | **compiler-rt** (auto-linked by clang) | ❌ |
| TLS callbacks (`__tls_used` / `.tls` directory), `__declspec(thread)` initialisation | **lld + OS loader** — the linker emits the TLS directory; the OS calls callbacks before `DllMain`/`_DllMainCRTStartup`.  Not a CRT concern. | ❌ |
| Per-thread DLL notifications (`DLL_THREAD_ATTACH`/`DETACH`): explicitly *disabled* at PROCESS_ATTACH via `DisableThreadLibraryCalls`; if a consumer needs them they must handle TLS callbacks instead | **OS**, but suppressed by Akari | ❌ (suppressed) |
| PE/COFF layout, subsystem selection, `/ENTRY` resolution, `.pdata`/`.xdata` EH tables | **lld** — pass `-Wl,-subsystem:windowsce:<ver>` and `-Wl,-entry:<name>` | ❌ |
| Win32 SDK headers (`windows.h`, `windef.h`, `winnt.h`), `coredll.lib` import library | Consumer's WinCE SDK / Platform Builder output | ❌ |

---

## Supported architectures and CE versions

Akari is written to be **architecture-neutral**.  All sources are C99 with a
small set of compiler attributes; there is no inline assembly, no endianness
assumption, no pointer-width assumption, and no calling-convention decorator
that hard-codes one ISA.

| Architecture | CE versions | coredll calling convention |
|---|---|---|
| ARM (v4, v4i, v5, v6, v7 Thumb/Thumb2, IWMMXT) | 4.0–6.0 (+7) | Platform default (APCS/AAPCS); no `__stdcall` |
| x86 (i486+, CEPC / DeviceEmulator) | 4.0–6.0 | **`__cdecl`** — *not* `__stdcall` like desktop Win32.  coredll symbols are undecorated; decorating them `__stdcall` produces `_Foo@4` lookups that fail to link. |
| MIPS (MIPSII, MIPSII_FP, MIPSIV, MIPSIV_FP, MIPS16) | 4.0–6.0 | Platform default; no `__stdcall` |
| SuperH (SH3, SH4) | 4.0–6.0 (H/PC, handhelds) | Platform default; no `__stdcall` |

The `WINAPI` macro in `include/akari/compiler.h` expands to
`__attribute__((stdcall))` **only** for desktop (non-CE) 32-bit x86
Win32.  For `_WIN32_WCE` and every other supported architecture it is
empty.

Select a target with `CROSS=`:

```sh
make CROSS=armv4-wince-     # ARMv4  (Pocket PC 2003 / Windows Mobile 2003)
make CROSS=armv5-wince-     # ARMv5  (CE 5 / Windows Mobile 5)
make CROSS=armv7-wince-     # ARMv7 Thumb2  (CE 6 / Windows Mobile 6.5)
make CROSS=i686-wince-      # x86    (CEPC / DeviceEmulator)
make CROSS=mips-wince-      # MIPS
make CROSS=sh4-wince-       # SuperH 4
```

---

## Build products and how to link them

```
build/libakari.a         Data globals (__argc/__argv/...), .init_array/
                         .fini_array runners, weak default WinMain/DllMain.
build/akari_crt0.o       ALL five EXE entry points.  Link this once into
                         any EXE.
build/akari_dllcrt.o     _DllMainCRTStartup.  Link this once into any DLL.
build/akari_crt0w.o      Backward-compat alias (byte-identical to akari_crt0.o).
build/akari_crt0c.o      Backward-compat alias (byte-identical to akari_crt0.o).
```

**Why `libakari.a` + separate startup objects, not a single archive?**
Because PE entry points are not referenced from anywhere else in the link
unit; if they lived only inside an archive the linker would not pull them
in without an explicit `-u <symbol>` or `/ENTRY` hint.  This matches the
layout of every other CRT (glibc `crt1.o`, msvcrt `crt0.obj`, mingwrt
`crt0.o`, ...): the entry-point object is linked explicitly, and the
archive provides everything else.

`akari_crt0.o` contains **all five entry points** in a single TU; the
linker discards unreferenced ones via `-ffunction-sections`/`--gc-sections`.
There is no longer any code duplication between `crt0.o`/`crt0w.o`/`crt0c.o`
— the `w`/`c` files are installed as byte-identical copies for makefile
compatibility only.

### Typical link line (ARMv7 Thumb2, CE 6 / Windows Mobile 6.x)

```sh
clang --target=thumbv7-unknown-windows-gnu \
      -fuse-ld=lld \
      -Wl,--gc-sections \
      -I<your-sdk>/include \
      -Wl,-subsystem:windowsce:9.0 \
      -Wl,-entry:WinMainCRTStartup \
      your_code.o \
      <prefix>/lib/akari_crt0.o \
      -L<prefix>/lib -lakari \
      -L<your-sdk>/lib -lcoredll \
      -lc++ -lc++abi -lunwind \
      -o your.exe
```

For a DLL replace `akari_crt0.o` with `akari_dllcrt.o`, drop
`-Wl,-entry:...` (the linker will find `_DllMainCRTStartup` by default),
and add `-shared`.

Subsystem versions:
| Value | OS |
|---|---|
| `-Wl,-subsystem:windowsce:4.0` | Windows CE 4.0 / .NET |
| `-Wl,-subsystem:windowsce:5.0` | Windows CE 5.0 |
| `-Wl,-subsystem:windowsce:5.01` | Windows CE 5.01 (Windows Mobile 5) |
| `-Wl,-subsystem:windowsce:5.02` | Windows CE 5.02 (Windows Mobile 6 Standard) |
| `-Wl,-subsystem:windowsce:9.0`  | Windows CE 6.x / Windows Mobile 6.x |

x86 CE: replace `--target=thumbv7-unknown-windows-gnu` with
`--target=i686-unknown-windows-gnu`; all other flags are identical.

Notes:
* Do not pass `-nostdlib` unless you also explicitly link
  `libclang_rt.builtins-<arch>.a` — clang adds compiler-rt automatically
  and Akari needs `__chkstk`, `__aeabi_*`, etc., from it.
* Stack protection (`-fstack-protector`) must NOT be enabled for Akari's
  own startup files (the Makefile already passes `-fno-stack-protector`);
  your application code can use it freely — `__stack_chk_guard` and
  `__stack_chk_fail` come from libc.
* `__cxa_atexit` / `__cxa_finalize` are part of libc++abi and libc.
  Akari calls libc `exit()`, which is responsible for calling
  `__cxa_finalize(NULL)` to run all registered static destructors before
  invoking `ExitProcess`.  You do not need any extra glue.

### Host-side build check

```sh
make hostcheck
```

compiles both TUs warning-free with host `gcc` at `-Wall -Wextra -Wshadow
-Wstrict-prototypes -Wmissing-prototypes -std=c99` and archives them.

---

## Repository layout

```
include/akari/compiler.h   Compiler macros (NORETURN/WEAK/USED/AKARI_DLLIMPORT/WINAPI).
                           WINAPI expands to nothing on Windows CE.
include/akari/crt.h        Declares the MSVCRT data globals Akari defines.
src/crt/crt0.c             EXE entry points: WinMainCRTStartup / wWinMainCRTStartup /
                           mainCRTStartup / mainWCRTStartup / mainACRTStartup, plus
                           CommandLineToArgvW-compatible parser, WideCharToMultiByte
                           narrow-argv synthesis, .init_array/.ctors dispatch.
src/crt/dllcrt.c           DLL entry point: _DllMainCRTStartup with DisableThreadLibraryCalls,
                           .init_array on ATTACH, .fini_array on DETACH, weak DllMain.
```

---

## Per-topic design notes

### `WinMain` `lpCmdLine` is the raw tail, not a re-joined argv

Desktop Win32 and Windows CE both pass `lpCmdLine` as a pointer into the
raw `GetCommandLineW()` buffer, pointing at the first unquoted whitespace
*after* the program-name token — preserving whatever arbitrary spacing
the launcher supplied.  Akari reproduces this exactly: `_wcmdtail` is
set by rescanning the raw command line past argv[0] without allocating
or copying.  If you want a tokenised argument list use `__argc`/`__wargv`.

### Backslash/quote parsing

The parser implements CommandLineToArgvW exactly:
* `2n` backslashes before `"` → `n` literal backslashes, toggle quoting.
* `2n+1` backslashes before `"` → `n` backslashes + one literal `"`.
* `""` *inside* a quoted range → one literal `"`.
* Whitespace outside quotes separates arguments.

### ANSI code-page conversion for `__argv`

`__argv` / `_acmdln` are narrow (`char`) strings for source compatibility
with legacy code that expects `main(int, char **)`.  They are converted
with `WideCharToMultiByte(CP_ACP)` if the function is available from
coredll; if it is not (kernel-only headless images) we fall back to a
lossy 7-bit-passthrough, non-ASCII→`'?'` conversion.  New code should
always walk `__wargv` for correct Unicode handling.

### envp

Windows CE has no POSIX environment block — no `environ`, no
`getenv`/`setenv`, no `GetEnvironmentStrings`.  The third argument to
`main` is always `NULL`.

### `.init_array` / `.ctors` execution order

* `.init_array` entries are emitted in forward order and run from low
  address to high address (System V ABI).
* `.ctors` entries (legacy list) are emitted in reverse order under the
  GCC sentinel convention and are invoked from the end of the list
  backwards.
* `.fini_array` (DLL only) is invoked in reverse order (LIFO) on
  `DLL_PROCESS_DETACH`, matching `__cxa_atexit` semantics.

### DLL TLS callbacks and thread notifications

Akari calls `DisableThreadLibraryCalls` on `DLL_PROCESS_ATTACH`.  This
suppresses DLL_THREAD_ATTACH/DETACH calls for all subsequently created
threads — these notifications are slow on CE and the CRT needs no
per-thread bookkeeping.  Consumers that require per-thread hooks must
use **PE/COFF TLS callbacks** (declared via `__declspec(thread)` or the
`.tls` directory), which the OS invokes before `_DllMainCRTStartup`;
that mechanism is the linker's responsibility, not the CRT's.

### DLL_PROCESS_DETACH shutdown order

On detach we call `DllMain(hDll, DLL_PROCESS_DETACH, …)` FIRST, then run
`.fini_array` destructors in reverse order.  Destructors therefore see
all of the DLL's own code and data still mapped.  `__cxa_finalize` for
static C++ objects inside the DLL is the C++ runtime's responsibility
(libc++abi calls it from its own `.fini_array` callback or via a DllMain
hook).  Akari does not call `__cxa_finalize` directly because doing so
on process exit would double-run destructors (libc `exit()` already
invokes them).

### `setjmp`/`longjmp`

Provided by libc (coredll exports `_setjmp`/`longjmp` at fixed ordinals
on ARM CE; they are compatible with the ARM EHABI unwinding tables that
llvm-libunwind processes).  Akari does not redefine them.

### libc++abi / llvm-unwind on Windows CE ARM

libc++abi's ARM EHABI personality (`__gxx_personality_v0`) and
llvm-libunwind's `.pdata`/`.xdata` unwinder work on Windows CE as long
as the linker emits correct EH tables — lld for the `*-windows-gnu`
triple does.  No Akari code is in the exception path.

### lld subsystem and entry point

`-Wl,-subsystem:windowsce:<ver>` sets the PE Subsystem field to 9
(Windows CE) and the MajorSubsystemVersion/MinorSubsystemVersion to the
version you request.  `-Wl,-entry:<name>` selects the entry-point symbol;
Akari exposes all five canonical names.  There is no linker script.

---

## Clean-room status

Akari is written from scratch, clean-room, under the MIT license.  **No
code from msvcrt.dll, ucrt, mingw-w64, mingwrt, cegcc/mingw32ce,
w32api, newlib, glibc, dietlibc, or musl was copied, adapted, or
consulted during implementation.**  Public information sources were
limited to:

* ISO C99 / C11 and the Itanium C++ ABI specification.
* Microsoft's public documentation: PE/COFF specification, Win32 API
  reference for the handful of coredll entry points Akari calls
  (`GetModuleHandleW`, `GetCommandLineW`, `LocalAlloc`, `LocalFree`,
  `DisableThreadLibraryCalls`, `WideCharToMultiByte`), `WinMain`/
  `DllMain` signatures, and the CommandLineToArgvW parsing rules.
* Publicly visible coredll.dll ordinal listings, used only to confirm
  which symbols are exported by the OS and therefore must NOT be
  defined by Akari.

## License

MIT — see the copyright notice at the top of each source file and the
LICENSE file.

&copy; 2026 Akari CRT contributors.
