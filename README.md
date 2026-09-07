# Akari CRT (明かり) — Windows CE startup glue for Clang/LLVM

Akari is a small, architecture-neutral PE/COFF **startup layer** for
programs built with **Clang/lld** targeting **Windows CE 4.x, 5.x and 6.x**
(ARM, x86, MIPS, SuperH).  It gets an image from the PE entry point to the
user's `main` / `wmain` / `WinMain` / `wWinMain` / `DllMain`: it parses the
command line, runs global constructors, and hands shutdown to the C
library.

Akari is **not** a C library, **not** a C++ runtime, **not**
compiler-rt, and **not** an SDK or linker script.  No libc, libstdc++,
libc++ or STL code lives here; those responsibilities stay with the
components the consumer links (see the table below).  Akari deliberately
replaces only the role a MinGW-style runtime played for the
`*-windows-gnu` Clang toolchain.

## Responsibility table

| Concern | Provider | Akari ships it? |
|---|---|---|
| EXE entry points `WinMainCRTStartup`, `wWinMainCRTStartup`, `mainACRTStartup`, `mainWCRTStartup` (the four names Windows CE documents; there is **no** `mainCRTStartup` on CE — see below) | **Akari** (`akari_crt0.o`) | ✅ |
| x86-only legacy aliases `_WinMainCRTStartup`, `_wWinMainCRTStartup`, `_mainACRTStartup`, `_mainWCRTStartup` (leading-underscore spellings of the older CE x86 tools) | **Akari** | ✅ |
| DLL entry points `_DllMainCRTStartup` (canonical CE spelling) and `DllMainCRTStartup` (lld's default DLL-entry search name) | **Akari** (`akari_dllcrt.o`) | ✅ |
| Command-line parse into `__argc`/`__argv`/`__wargv`, `_wcmdtail` for `WinMain`'s `lpCmdLine`, per the documented MS parsing rules | **Akari** (clean-room parser; imports `GetCommandLineW`) | ✅ |
| Narrow `__argv`/`_acmdln`: `WideCharToMultiByte(CP_ACP)` resolved at runtime, lossy 7-bit fallback when the converter is absent from the OS image | **Akari** | ✅ |
| MSVCRT-model data globals `__argc __argv __wargv _acmdln _wcmdln _wcmdtail _fmode _doserrno _commode __dso_handle` (coredll exports no data objects) | **Akari** (`libakari.a`) | ✅ |
| C constructors: `.CRT$XI*` / `.CRT$XC*` (MS-style objects) first-to-last, then the lld GNU `.ctors` list; C destructors from the lld `.dtors` list | **Akari** | ✅ |
| Weak user-entry fallbacks (`DllMain` default in the DLL object; missing `main`/`WinMain` detected at runtime) | **Akari** | ✅ |
| x86 `___main` hook (i686 Clang makes `main()` call it; here it is a no-op because entry points already ran constructors) | **Akari** | ✅ |
| Process exit: call the C library's `exit()` when one is linked (atexit/`__cxa_finalize`/stdio flush happen there); direct `ExitProcess` only as a no-libc fallback | **Akari** (just the call) | ✅ |
| C library (`malloc`/`printf`/`exit`/`atexit`/`abort`/`memcpy`/`setjmp`/errno/...) | **libc** (coredll msvcrt exports, newlib, llvm-libc...) | ❌ |
| `__stack_chk_guard`/`__stack_chk_fail`, `__chkstk`, `__aeabi_*` builtins | **libc / compiler-rt** (clang links compiler-rt automatically) | ❌ |
| `__cxa_atexit`/`__cxa_finalize`, `__cxa_pure_virtual`, `new`/`delete`, EH personality, RTTI | **libc++ / libc++abi** | ❌ |
| `.pdata`/`.xdata` EH tables and unwinding | **lld / llvm-libunwind** | ❌ |
| TLS directory, TLS callbacks | **lld + OS loader** | ❌ |
| PE layout, subsystem header, `/ENTRY` resolution | **lld** | ❌ |
| SDK headers, `coredll.lib` | consumer's Windows CE SDK / Platform Builder | ❌ |

## Supported architectures and CE versions

All sources are C99, use no inline assembly, no endianness or pointer-width
assumptions, and no ISA-specific calling-convention decorations; clang does
the lowering.  `WINAPI` (in `include/akari/compiler.h`) is empty on
Windows CE (Microsoft documents CE entry functions as `__cdecl`; on ARM
`__cdecl`/`__stdcall` coincide, and on x86 CE the CRT entry names carry
no `@n` decoration), and expands to `__stdcall` only for desktop 32-bit
x86 builds of the same headers.

| Architecture | CE versions | status |
|---|---|---|
| ARM v4–v7 (ARM or Thumb state, incl. Thumb-2) | 4.x–6.x | built & link-verified (this toolchain) |
| x86 (i486+, CEPC / emulator) | 4.x–6.x | built & link-verified (this toolchain) |
| MIPS (MIPSII/IV) | 4.x–6.x | source-ready; not yet built (toolchain lacks the target in the verified snapshot) |
| SuperH (SH3/SH4) | 4.x–6.x | source-ready; not yet built (toolchain lacks the target in the verified snapshot) |

## Build products and how to link them

```
build/libakari.a         runtime.c: data globals, parser, narrow-argv
                         synthesis, ctor/dtor runners, ___main hook.
build/akari_crt0.o       EXE entry points (4 + 4 x86 aliases).
build/akari_dllcrt.o     DLL entry points + weak DllMain.
build/akari_crt0w.o, akari_crt0c.o
                         byte-identical copies of akari_crt0.o kept for
                         compatibility with older link lines.
```

Entry points live in explicitly-linked objects (not in the archive)
because nothing references them from within the link unit — this matches
how every CRT ships `crt0`-style objects.  The runtime support is the
archive, linked by both EXEs and DLLs.

### Building

```sh
make                                # TARGET defaults to armv7-unknown-windows-gnu
make TARGET=i686-unknown-windows-gnu
make TARGET=armv7-unknown-windows-gnu ARCHFLAGS=-mthumb   # Thumb-2
make CC=/path/to/clang AR=/path/to/llvm-ar                # non-PATH tools
```

### Linking an EXE (verified end-to-end with clang + lld 22.1.8)

```sh
clang --target=armv7-unknown-windows-gnu -fuse-ld=lld -nostdlib \
      -Wl,--subsystem=windowsce:5.02 -Wl,--entry=mainACRTStartup \
      your_code.o akari_crt0.o -L. -lakari -L<sdk>/lib -lcoredll \
      -o your.exe
```

* Choose the entry by what the app defines: `mainACRTStartup` for
  `main()`, `mainWCRTStartup` for `wmain()`, `WinMainCRTStartup` /
  `wWinMainCRTStartup` for `WinMain` / `wWinMain`.  Any of the four works
  regardless of the user function (each falls through to the others),
  but the documented convention pairs them as above.
* There is deliberately **no** `mainCRTStartup`: Microsoft's CE
  documentation ("Linking to the CRT", "/ENTRY", Windows CE 5.0) assigns
  `main()` programs to `mainACRTStartup`.
* A DLL links `akari_dllcrt.o` instead and can omit `--entry` — lld's
  default DLL-entry search finds `DllMainCRTStartup`.
* If you do not link a C library, `exit` is unresolved-weak and the
  entry falls back to `ExitProcess` directly.

Subsystem: pass `--subsystem=windowsce:<ver>` (verified: 5.02 → header
`IMAGE_SUBSYSTEM_WINDOWS_CE_GUI`, major 5, minor 2).  Use 4.0/5.0/5.01/
5.02/9.0 for CE 4.0/5.0/5.01/5.02/6.x images.

## Host-side checks (no cross toolchain needed)

```sh
make check      # hostcheck + hosttest
```

`hostcheck` compiles every TU warning-free with the host `cc` and
archives them.  `hosttest` builds and **runs** `tests/host/test_main.c`
together with `runtime.c` (host stubs stand in for the coredll imports):
it feeds the parser the documented examples (quotes, backslash
`2n`/`2n+1`-before-quote, `""`-inside-quotes, unterminated quotes,
leading whitespace, empty command line → executable path) and checks
`__argc`/`__wargv`/`__argv`/`_wcmdtail` plus both wide→narrow conversion
modes — currently `all 90 checks passed`.

## Repository layout

```
include/akari/compiler.h   GNU/Clang attribute macros, AKARI_ENTRY asm-label
                           pinning, AKARI_DLLIMPORT, WINAPI (empty on CE),
                           AKARI_CPU_X86, NULL.
include/akari/crt.h        PUBLIC data-global declarations (MSVCRT model).
include/akari/internal.h   PRIVATE shared declarations (not installed).
src/crt/runtime.c          Data globals, command-line parser + narrow-argv
                           synthesis, .CRT$X* bookends, GNU-list runners,
                           weak ___main hook, coredll import declarations.
src/crt/crt0.c             EXE entries + x86 underscore aliases; weak user
                           main/WinMain detection; dtors + exit hand-off.
src/crt/dllcrt.c           _DllMainCRTStartup + DllMainCRTStartup alias,
                           weak DllMain default.
tests/host/test_main.c     Host parser self-test (see above).
```

## Design notes

### Entry dispatch and startup order (EXE)

Each EXE entry runs: parse command line → run constructors → call user
function → run destructors → hand off to the C library's `exit()` (or
fall back to `ExitProcess`).  `envp` is always `NULL` (Windows CE has no
POSIX environment block).  `WinMain`'s `lpCmdLine` is `_wcmdtail`: a
pointer into the raw wide command line just past the argv[0] token
(Win32 convention — not a re-joined copy of argv), and `hPrevInstance`
is `0` per the CE WinMain documentation.

### DLL attach/detach order

Per Microsoft's CE "Run-time Library Behavior" documentation: on
`DLL_PROCESS_ATTACH`, global constructors run **first**, then the user
`DllMain`; on `DLL_PROCESS_DETACH`, the user `DllMain` runs first, then
the destructors (the documented reverse).  `__dso_handle` is set to the
module handle on attach so destructor registration can be scoped to the
image.  Thread attach/detach and unknown reasons are forwarded to
`DllMain`; the CRT does no per-thread work of its own.

### Command-line parsing rules

The parser implements the rules Microsoft documents for command-line
parsing (CommandLineToArgvW / "Parsing C command-line arguments"):
space/tab delimit arguments outside quotes; inside quotes whitespace is
ordinary text; `2n` backslashes before `"` yield `n` backslashes and a
mode-toggling quote; `2n+1` yield `n` backslashes plus a literal `"`;
`""` inside a quoted region is one literal quote; an unterminated quoted
region runs to end of string; a line starting with whitespace yields an
empty argv[0]; an empty command line yields argv[0] = the executable's
full path (CommandLineToArgvW semantics).  A counting pass and a
materializing pass share one code path so they cannot disagree.  Storage
comes from `LocalAlloc(LPTR)` (one block per image, no libc).

### Wide and narrow argv

Windows CE is Unicode-native; `__wargv`/`_wcmdln` are the raw wide
forms.  `__argv`/`_acmdln` are synthesized with
`WideCharToMultiByte(CP_ACP)` resolved through `GetModuleHandleW(L"coredll.dll")`
+ `GetProcAddress` (some OEM images cut optional modules); when the
converter is missing, a lossy 7-bit passthrough (`>0x7F` → `'?'`) is
used instead, and both paths are exercised by the host self-test.

### Constructor/destructor lists (verified layout)

Clang for `*-windows-gnu` emits GNU-style `.ctors`/`.dtors` sections
(not `.init_array`/`.fini_array`).  lld concatenates the per-object
sections in link order, bracketed by a `-1` header and a `0` terminator,
and defines `__CTOR_LIST__`/`__DTOR_LIST__`.  Verified with
`llvm-readobj`/`llvm-objdump` on i686 and ARMNT images (one and two
objects, one and several entries per object): per-object words are
stored in **reverse source order**.  Akari therefore walks
`__CTOR_LIST__` **backward** (per-object source order; objects in
reverse link order — the historical GNU convention) and `__DTOR_LIST__`
**forward**, which is the exact mirror: destructors always run
last-constructed-first, both within an object and across objects.
Because lld points the list symbol at the `-1` header on i686 but at the
first real word on ARMNT (several entries), the walker skips a `-1`
only when it is actually at `l[0]`; every observed shape is covered.

MS-style objects (clang-cl, `-fms-compatibility`) place initializer
pointers in `.CRT$XI*`/`.CRT$XC*` sections instead.  Akari ships the
`.CRT$XIA`/`.CRT$XIZ` and `.CRT$XCA`/`.CRT$XCZ` NULL bookends in
`runtime.c` and walks those ranges first-to-last (lld orders the
`.CRT$X*` family by section name).  Mechanisms are no-ops when their
tables are empty, so mixed-style links stay well-defined.

### Stack protection, builtins, C++

Akari compiles its own TUs with `-fno-stack-protector` (no guard exists
before the C library initializes it) and `-fno-builtin` +
`-nostdlibinc` (it is the layer below any hosted runtime).  Note: do
**not** add `-ffreestanding` — Clang 22.x ARM `windows-gnu` emits
invalid `.seh` sequences for `-ffreestanding` code at `-Os` (verified;
see below).  `__cxa_atexit`/`__cxa_finalize` and static-destructor
finalization belong to libc++abi/libc; Akari's destructor runner only
covers the `.dtors` list Clang emits for GNU-style C `__attribute__((destructor))`
and (with libc++abi) never double-runs `__cxa_finalize`.

### Empty constructor functions

Clang at `-Os` removes empty `__attribute__((constructor))` functions
together with their `.ctors` entries (observed in IR: the
`llvm.global_ctors` array becomes empty).  This is normal optimization
and irrelevant to real code, whose constructors have side effects; it
only matters if you probe list layouts with empty bodies (use `-O0` or
side-effecting bodies).

## Clean-room status

Akari is written from scratch under the MIT license.  No code from
msvcrt/ucrt, MinGW-w64/mingwrt, CeGCC/mingw32ce, w32api, newlib, glibc,
musl, or any third-party OSS CRT was copied, ported, or adapted, and no
third-party code was combined into these implementations.  Design
decisions are grounded in official public information only:

* Microsoft's public documentation: PE/COFF format; "Linking to the CRT
  (Windows CE 5.0)", "/ENTRY (Windows CE 5.0)", "Run-time Library
  Behavior (Windows CE 5.0)", `WinMain`/`DllMain`/`GetCommandLine`
  (CE), `CommandLineToArgvW`/parsing rules, `WideCharToMultiByte`,
  `LocalAlloc`/`LocalFree`;
* ARM/LLVM/Clang/LLD documentation and the observable output of the
  official toolchain (see below).

### Verified toolchain behavior (clang/LLD 22.1.8 snapshot)

* All TUs compile warning-free at `-Wall -Wextra -Wshadow
  -Wstrict-prototypes -Wmissing-prototypes` for `armv7-unknown-windows-gnu`
  (ARM and `-mthumb` Thumb-2) and `i686-unknown-windows-gnu`.
* End-to-end links (clang driver and direct `ld.lld`):
  * EXE: `crt0.o + runtime.o + coredll import lib` →
    `IMAGE_FILE_MACHINE_ARMNT`/`I386`, subsystem `WINDOWS_CE_GUI` (9),
    5.02; imports exactly `ExitProcess` + `GetCommandLineW`,
    `GetModuleFileNameW`, `GetModuleHandleW`, `GetProcAddress`,
    `LocalAlloc`, `LocalFree` — no desktop-API imports.
  * DLL: `dllcrt.o + runtime.o` links with the default entry search
    (`DllMainCRTStartup`), no `ExitProcess` import.
* Entry resolution, `___main` injection on i686, and the list layouts
  described above were confirmed with `llvm-readobj` on the linked
  images.
* The host self-test (`make hosttest`) runs the parser against the
  documented rules on the host.

## Known verification gaps

* MIPS/SH: not yet built in this snapshot (no such target in the
  downloaded LLVM build); the sources contain no ISA-specific code.
* No Windows CE device/emulator runtime was available: entry-point
  execution is verified to the OS loader boundary (headers, entry
  symbols, imports, list layouts), not by running images on CE 4/5/6.
* Per-CE-version coredll export coverage of the imported APIs is
  audited against official SDK documentation; images that cut optional
  modules are handled defensively at runtime (see the `WideCharToMultiByte`
  fallback above).

## License

MIT — see the notice at the top of each file and the LICENSE file.

© 2026 Akari CRT contributors.
