# Akari CRT (明かり) — Windows CE startup glue for Clang/LLVM

Akari is a small, architecture-neutral PE/COFF **startup layer** for
programs built with **Clang/lld** targeting **Windows CE 4.x, 5.x and 6.x**
on 32-bit ARM (ARM/Thumb state, including the `armel` ABI used by the
WinCE toolchain), x86, MIPS and SuperH.  It gets an image from the PE
entry point to the user's `main` / `wmain` / `WinMain` / `wWinMain` /
`DllMain`: it parses the command line, runs global constructors, and
hands shutdown to the C library.

Akari is **not** a C library, **not** a C++ runtime, **not**
compiler-rt, and **not** an SDK or linker script.  No libc, libstdc++,
libc++ or STL code lives here; those responsibilities stay with the
components the consumer links (see the table below).  Akari deliberately
covers only the startup/process-glue role of the CRT runtime that
accompanies Clang/lld on these targets.

## Toolchain

Akari is built and link-verified with the Windows CE toolchain of
[kagurasumusun/llvm-project](https://github.com/kagurasumusun/llvm-project)
(branch `LLVM-WinCE`): clang/lld with a WinCE driver, COFF/CE support in
lld, and the `*-pc-wince` target triple (`arm-pc-wince` and
`i386-pc-wince`, optionally versioned as `arm-pc-wince5.0` /
`arm-pc-wince4.2` / ...).  In that toolchain the ARM target defaults to
the **`armel` ABI**: ARMv5TE (`arm926ej-s`), little-endian, AAPCS
soft-float (`-mfloat-abi=soft`); clang predefines `_WIN32_WCE`,
`UNDER_CE`, `__ARMEL__`, `__ARM_PCS` etc. for it.  CE deployment
versions select the coredll import surface and the `_WIN32_WCE` value.

Plain `*-windows-gnu` triples (`armv7-unknown-windows-gnu`,
`i686-unknown-windows-gnu`, Thumb-2 with `-mthumb`) remain supported
and verified targets of the same sources, for toolchains without the
WinCE driver.

## Responsibility table

| Concern | Provider | Akari ships it? |
|---|---|---|
| EXE entry points `WinMainCRTStartup`, `wWinMainCRTStartup`, `mainACRTStartup`, `mainWCRTStartup` (the four names Windows CE documents; there is **no** `mainCRTStartup` on CE — see below) | **Akari** (`akari_crt0.o`) | ✅ |
| x86-only aliases `_WinMainCRTStartup`, `_wWinMainCRTStartup`, `_mainACRTStartup`, `_mainWCRTStartup` (the leading-underscore spellings an x86 C compiler gives these C names) | **Akari** | ✅ |
| DLL entry points `_DllMainCRTStartup` (canonical CE spelling) and `DllMainCRTStartup` (the WinCE driver's `/entry:DllMainCRTStartup`) | **Akari** (`akari_dllcrt.o`) | ✅ |
| Command-line parse into `__argc`/`__argv`/`__wargv`, `_wcmdtail` for `WinMain`'s `lpCmdLine`, per the documented MS parsing rules | **Akari** (clean-room parser; imports `GetCommandLineW`) | ✅ |
| Narrow `__argv`/`_acmdln`: `WideCharToMultiByte(CP_ACP)` resolved at runtime, lossy 7-bit fallback when the converter is absent from the OS image | **Akari** | ✅ |
| MSVCRT-model data globals `__argc __argv __wargv _acmdln _wcmdln _wcmdtail _fmode _doserrno _commode __dso_handle` (coredll exports no data objects) | **Akari** (`libakari.a`) | ✅ |
| C/C++ initializers: `.CRT$XI*` / `.CRT$XC*` (clang-cl objects) first-to-last, then the `.ctors` list (`__CTOR_LIST__`); destructors from the `.dtors` list | **Akari** | ✅ |
| Weak user-entry fallbacks (`DllMain` default in the DLL object; missing `main`/`WinMain` detected at runtime) | **Akari** | ✅ |
| x86 `___main` hook (only i686 `windows-gnu` clang makes `main()` call it; here it is a no-op because entry points already ran constructors) | **Akari** | ✅ |
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
the lowering.  The same objects serve the `armel` (soft-float AAPCS) and
hard-float/other-ABI ARM targets, x86, and any other CE target clang can
lower: only clang's target code generation and lld's COFF handling differ
between them.  `WINAPI` (in `include/akari/compiler.h`) is empty on
Windows CE (Microsoft documents CE entry functions as `__cdecl`; on ARM
`__cdecl`/`__stdcall` coincide, and on x86 CE the CRT entry names carry
no `@n` decoration), and expands to `__stdcall` only for desktop 32-bit
x86 builds of the same headers.

| Architecture / ABI | Triple (this toolchain) | CE versions | status |
|---|---|---|---|
| ARMv5TE **armel** (little-endian, soft-float AAPCS; the WinCE default) | `arm-pc-wince`, `arm-pc-wince5.0`, `arm-pc-wince4.2`, ... | 4.x–6.x | built & link-verified (EXE + DLL) |
| x86 (CEPC / emulator) | `i386-pc-wince` | 4.x–6.x (coredll surface per version) | built & link-verified |
| ARMv7 (ARM/Thumb-2) | `armv7-unknown-windows-gnu` (`-mthumb`) | 5.x–6.x class | built & link-verified (windows-gnu) |
| x86 | `i686-unknown-windows-gnu` | 4.x–6.x | built & link-verified (windows-gnu) |
| MIPS (MIPSII/IV) | — | 4.x–6.x | source-ready; not buildable with this toolchain (no such target) |
| SuperH (SH3/SH4) | — | 4.x–6.x | source-ready; not buildable with this toolchain (no such target) |

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
make                                # TARGET defaults to arm-pc-wince (armel, CE 6.0)
make TARGET=arm-pc-wince5.0         # CE 5.0 deployment (driver defines _WIN32_WCE=0x500)
make TARGET=i386-pc-wince           # x86 CE
make TARGET=armv7-unknown-windows-gnu ARCHFLAGS=-mthumb   # Thumb-2 (windows-gnu)
make CC=/path/to/clang AR=/path/to/llvm-ar                # non-PATH tools
```

### Linking an EXE (verified end-to-end)

The WinCE clang driver always prepends its own start file (`crt3.o`)
and the CeGCC-lineage archives (`libmingw32.a`, `libcoredll*.a`, ...)
and ignores `-nostdlib`/`-nostartfiles` (verified), so a link that uses
Akari drives `lld-link` directly, with `-wince` (CE image defaults):

```sh
lld-link -wince /subsystem:windowsce /entry:WinMainCRTStartup \
    /base:0x10000 /fixed \
    your_code.o akari_crt0.o libakari.a <sysroot>/lib/libcoredll6.a \
    -o your.exe
```

* Choose the entry by what the app defines: `mainACRTStartup` for
  `main()`, `mainWCRTStartup` for `wmain()`, `WinMainCRTStartup` /
  `wWinMainCRTStartup` for `WinMain` / `wWinMain`.  Any of the four works
  regardless of the user function (each falls through to the others),
  but the documented convention pairs them as above.  The toolchain's
  own default entry is `WinMainCRTStartup` (EXE) and
  `DllMainCRTStartup` (DLL), both of which Akari provides.
* There is deliberately **no** `mainCRTStartup`: Microsoft's CE
  documentation ("Linking to the CRT", "/ENTRY", Windows CE 5.0) assigns
  `main()` programs to `mainACRTStartup`.
* The coredll import library is the version- and architecture-selected
  sysroot archive: CE 6.0 `libcoredll6.a` (x86: `libcoredll6-x86.a`),
  CE 5.0 `libcoredll.a`, CE 4.x `libcoredll4.a` (toolchain driver
  behavior; an SDK's own `coredll.lib` works the same way).  The
  toolchain's `wince-sysroot` is assembled by
  `kagurasumusun/cellvm-build`'s `build-wince-sysroot.sh`.
* The compiler-rt builtins archive (`libclang_rt.builtins-*.a`) and, for
  C++, libc++/libc++abi/libunwind remain the compiler's/libraries'
  responsibility (see the table above).
* `-auto-import` is **not** needed: Akari imports coredll functions with
  explicit `__declspec(dllimport)`, so no pseudo-relocation table is
  emitted and no `_pei386_runtime_relocator` is required.  (Legacy
  CeGCC objects that reference DLL data without dllimport would need
  the mingwrt-style runtime support instead — outside Akari's scope.)
* If you do not link a C library, `exit` is unresolved-weak and the
  entry falls back to `ExitProcess` directly (weak externs resolve to
  zero in lld-link; verified).

Subsystem: `lld-link -wince` accepts `/subsystem:windowsce` and stamps
subsystem 9 (`IMAGE_SUBSYSTEM_WINDOWS_CE_GUI`) with OS version 6.0 in
the PE optional header (verified).  Versioned spellings
(`/subsystem:windowsce:5.02`) are rejected by lld-link (verified).

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
                           synthesis, .CRT$X* bookends, .ctors/.dtors list
                           runners, weak ___main hook, coredll imports.
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

Clang emits `.ctors`/`.dtors` COFF sections for `__attribute__((constructor))`
/ `__attribute__((destructor))` on every CE-capable target of the
verified toolchain (`armv7-unknown-windows-gnu`, `i686-unknown-windows-gnu`,
`arm-pc-wince`, `i386-pc-wince`).  The linker concatenates the per-object
contents in link order, bracketed by a `-1` header and a `0` terminator,
and defines `__CTOR_LIST__`/`__DTOR_LIST__` pointing at the `-1` header.
Verified with `llvm-readobj`/`llvm-objdump` on linked images from both
linkers of the toolchain — ld.lld (windows-gnu, i686 and ARMNT objects)
and lld-link in `-wince` mode (armel and x86 CE objects), one and two
objects, one and several entries per object, with strong and weak
references.  Per-object word order is target-dependent (verified on
objects): `*-windows-gnu` Clang stores `.ctors`/`.dtors` words in
**reverse source order**, while the WinCE driver's `*-pc-wince` targets
store them in **source order**.  Windows CE documentation does not
specify static-initializer order across objects, so Akari's documented
choice is: walk `__CTOR_LIST__` **backward** and `__DTOR_LIST__`
**forward**.  Because the `.dtors` blocks are stored symmetrically to
the `.ctors` blocks, destruction is then the exact mirror of
construction in both storage orders — last-constructed entries run
first, both inside one object and across objects — and no runtime
bookkeeping is needed.  The walkers tolerate a list that starts
directly with a real entry, skipping the `-1` only when it is actually
present at `l[0]`.

Objects compiled in MS style (clang-cl) place initializer pointers in
`.CRT$XI*`/`.CRT$XC*` sections instead.  Microsoft documents that its
linker combines these subsections in the order of the part after `$`
("CRT initialization", Microsoft Learn), so user entries in
`.CRT$XCU`/`.CRT$XIU` always land between the `.CRT$XCA`/`.CRT$XCZ`
and `.CRT$XIA`/`.CRT$XIZ` pairs.  Akari ships those four NULL sentinels
in `runtime.c` and walks the ranges first-to-last.  lld-link in `-wince`
mode merges the family into one `.CRT` section sorted by subsection
name — verified on linked armel images containing wince-crt objects and
clang-cl objects: the layout is exactly `XCA(NULL) XCU(entry)
XCZ(NULL) XIA(NULL) XIZ(NULL) [XTU...]` and no other data falls inside
the walked ranges.  (The same ordering was verified for ld.lld on
windows-gnu images.)  Mechanisms are no-ops when their tables are
empty, so mixed-style links stay well-defined.

### Stack protection, builtins, C++

Akari compiles its own TUs with `-fno-stack-protector` (no guard exists
before the C library initializes it) and `-fno-builtin` +
`-nostdlibinc` (it is the layer below any hosted runtime).  Note: do
**not** add `-ffreestanding` — the ARM `windows-gnu` backend of the
verified toolchain emits invalid `.seh` directives for
`-ffreestanding` code at `-Os` (verified); the `*-pc-wince` target is
not affected.  `__cxa_atexit`/`__cxa_finalize` and static-destructor
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
  verified toolchain (see below).

### Verified toolchain behavior (kagurasumusun/llvm-project, branch LLVM-WinCE)

All items below were verified with the toolchain's clang/lld build from
CI run `34078339236` (head `29d8b882ab`, clang 22.1.8 with the WinCE
driver and COFF/CE lld support).

* All TUs compile warning-free at `-Wall -Wextra -Wshadow
  -Wstrict-prototypes -Wmissing-prototypes` for `arm-pc-wince`
  (armel: ARMv5TE, soft-float AAPCS — `__ARMEL__`, `__ARM_PCS`,
  `__ARM_EABI__` predefined), `arm-pc-wince5.0`/`4.2` (driver defines
  `_WIN32_WCE` 0x500/0x420 with no warning when the CRT build omits its
  own `-D`), `i386-pc-wince`, and for `armv7-unknown-windows-gnu`
  (ARM and `-mthumb` Thumb-2) and `i686-unknown-windows-gnu`.
* Objects: machine `IMAGE_FILE_MACHINE_ARM` (0x1C0) for `arm-pc-wince`,
  `IMAGE_FILE_MACHINE_I386` for `i386-pc-wince`; entry symbols
  `WinMainCRTStartup`/`wWinMainCRTStartup`/`mainACRTStartup`/
  `mainWCRTStartup` plus, on x86 only, their leading-underscore
  aliases; `.CRT$XIA/XIZ/XCA/XCZ` sentinel sections present.
* End-to-end links with direct `lld-link -wince` (the driver's own
  link mode; it always prepends its CeGCC start files and ignores
  `-nostdlib`, so Akari links bypass it):
  * EXE (`crt0.o + runtime.o + user + link stubs`): machine ARM/I386,
    subsystem `WINDOWS_CE_GUI` (9), OS version 6.0, entry resolved to
    the requested CE entry; `.ctors` = `[-1, entries..., 0]`, `.CRT` =
    the four NULL sentinels (plus sorted user entries when a clang-cl
    object is present); no desktop-API imports beyond the coredll set.
  * DLL (`dllcrt.o + runtime.o`, `/dll /entry:DllMainCRTStartup`):
    links; subsystem 9.
* `lld-link` rejects versioned `/subsystem:windowsce:5.02` spellings;
  bare `/subsystem:windowsce` stamps version 6.0 (verified).
* `___main`: i686 `windows-gnu` clang injects a call into `main()`
  (so Akari keeps its no-op `___main`); `i386-pc-wince` does not
  (verified on objects).
* clang-cl objects for `arm-pc-wince` put initializer pointers in
  `.CRT$XCU`/`.CRT$XTU` (use `/Zl` to drop the `.drectve` default-lib
  lines); lld-link merges them into the sorted `.CRT` layout above
  (verified).
* `.CRT$X*`/list layouts were verified in both object orders (user
  object before and after the CRT objects) with identical offsets.
* Per-object `.ctors`/`.dtors` storage order differs between the two
  target families of the same clang: `*-windows-gnu` stores words in
  reverse source order, `*-pc-wince` in source order (verified on
  objects with several entries); the backward-ctor/forward-dtor
  runners preserve the LIFO mirror under both (see "Constructor/
  destructor lists").
* The host self-test (`make hosttest`) runs the parser against the
  documented rules on the host.

## Known verification gaps

* MIPS/SH: the WinCE clang driver accepts only arm/thumb/x86 triples
  (it rejects every other architecture with an explicit diagnostic,
  and LLVM has no SuperH backend); the sources contain no ISA-specific
  code and remain ready for a toolchain that can target MIPS/SH CE.
* No Windows CE device/emulator runtime was available: entry-point
  execution is verified to the OS loader boundary (headers, entry
  symbols, imports, list layouts), not by running images on CE 4/5/6.
* `lld-link -wince` stamps subsystem OS version 6.0 unconditionally
  (versioned `/subsystem:windowsce:` spellings are rejected); whether
  CE 4/5 loaders accept a 6.0 stamp is not verified on hardware — if a
  CE 4/5 device rejects it, the fix belongs in the linker's CE support
  (a header patch is outside Akari's scope).
* Per-CE-version (4.x/5.x/6.x) export coverage of the imported APIs
  against official SDK documentation is a pending audit; images that
  cut optional modules are handled defensively at runtime (see the
  `WideCharToMultiByte` fallback above).

## License

MIT — see the notice at the top of each file and the LICENSE file.

© 2026 Akari CRT contributors.
