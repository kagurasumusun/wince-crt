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
| C/C++ initializers: `.CRT$XI*` / `.CRT$XC*` (MS-style objects) first-to-last, then the `.ctors` list (`__CTOR_LIST__`); destructors from the `.dtors` list | **Akari** | ✅ |
| Weak user-entry fallbacks (`DllMain` default in the DLL object; missing `main`/`WinMain` detected at runtime) | **Akari** | ✅ |
| x86 `___main` hook (only i686 `windows-gnu` clang makes `main()` call it; here it is a no-op because entry points already ran constructors) | **Akari** | ✅ |
| Process exit: call the C library's `exit()` when one is linked (atexit/`__cxa_finalize`/stdio flush happen there); direct `TerminateProcess` on the current process only as a no-libc fallback (coredll exports no `ExitProcess` on any CE generation) | **Akari** (just the call) | ✅ |
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
  entry terminates the process directly via `TerminateProcess` (weak
  externs resolve to zero in lld-link; verified).  coredll exports no
  `ExitProcess` on any CE generation (see "coredll import surface"
  below), so the fallback uses the CE-native termination call.

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
fall back to `TerminateProcess` on the current process).  `envp` is always `NULL` (Windows CE has no
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
+ `GetProcAddressW` (coredll exports `GetProcAddress` only in its W
spelling; some OEM images cut optional modules); when the
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

Objects compiled in MS style (code that allocates
`.CRT$XI*`/`.CRT$XC*` sections, e.g. `__declspec(allocate(...))`, or
objects from MS-compatible toolchains) place initializer pointers in
those sections instead.  Microsoft documents that its linker combines
these subsections in the order of the part after `$` ("CRT
initialization", Microsoft Learn), so user entries in
`.CRT$XCU`/`.CRT$XIU` always land between the `.CRT$XCA`/`.CRT$XCZ`
and `.CRT$XIA`/`.CRT$XIZ` pairs.  Akari ships those four NULL sentinels
in `runtime.c` and walks the ranges first-to-last.  lld-link in `-wince`
mode merges the family into one `.CRT` section sorted by subsection
name — verified on linked i386-pc-wince and arm-pc-wince images with
`__declspec(allocate(".CRT$XCU"))`/`.CRT$XIU` user entries placed in
both object orders: the layout is exactly `XCA(NULL) XCU(entry)
XCZ(NULL) XIA(NULL) XIU(entry) XIZ(NULL)` with identical offsets in
both orders, and no other data falls inside the walked ranges.  (The
same ordering was verified for ld.lld on windows-gnu images.)
Mechanisms are no-ops when their tables are empty, so mixed-style
links stay well-defined.

Note that the verified toolchain's own clang-cl (both `i386-pc-wince`
and `arm-pc-wince`) does NOT emit `.CRT$XCU` for plain C++ static
initializers: it emits GNU-style `.ctors` entries (`_GLOBAL__sub_I_*`,
observed in the objects) and registers static destructors through
`atexit` — those run through `__CTOR_LIST__`/`__DTOR_LIST__` and the
C library's `atexit`, respectively.  `.CRT$X*` is therefore exercised
only when a link contains explicitly allocated MS-style sections.

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

### Official-source cross-check (MSDN / MS Learn, Windows CE archives)

Every CRT/startup/link record in this repository was re-checked against
the official Windows CE documentation as published by Microsoft (the
CE-era MSDN pages, now served as *Windows Embedded / MSDN archive*
under learn.microsoft.com; URLs below are the canonical archive IDs).
Result: every fetched page agrees with the records stated above, with
**one documented conflict** — the official *ExitProcess* page
(`ms885217`) claims a Coredll.lib export that no CE 4/5/6 import
library provides (see the last table row and the notes below).  The
conflict changes no code: Akari never references `ExitProcess`.  Claim
→ source correspondence:

| Record in this repository | Official source (all fetched in full) |
|---|---|
| CRT = `COREDLL.DLL` (via import library `COREDLL.LIB`) + `CORELIBC.LIB` (static CRT startup and performance-critical routines) | *Linking to the CRT (Windows CE 5.0)*, MSDN archive ID `ms859584` — `learn.microsoft.com/en-us/previous-versions/windows/embedded/ms859584(v=msdn.10)`; also *C Run-time Libraries (Windows CE 5.0)* overview, `ms859579` (“The Coredll.lib and Corelibc.lib library files contain the C run-time library functions”) |
| The five CE entry aliases, exact spellings: `mainACRTStartup`, `mainWCRTStartup`, `WinMainCRTStartup`, `wWinMainCRTStartup`, `_DllMainCRTStartup`; EXE default entry `wWinMainCRTStartup`, else `WinMainCRTStartup`; DLL default `_DllMainCRTStartup`; mainA/mainW require an explicit `/ENTRY` | `ms859584` (same page, “CRT Startup Functions”) |
| DLL startup sequence: `_DllMainCRTStartup` initializes CRT and DLL, runs constructors of static/nonlocal C++ objects, then calls user `DllMain(PROCESS_ATTACH)`; on process completion, user `DllMain(PROCESS_DETACH)` then the termination list (atexit functions + global/static-object destructors); attach order is the reverse of detach order; thread attach/detach are delivered to the entry but the CRT does no initialization/termination for them; every process gets its own copy of DLL data | *Run-time Library Behavior (Windows CE 5.0)*, `ms859588` |
| CE `WinMain` is `int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)` with `hPrevInstance` always NULL (use a uniquely named mutex + `ERROR_ALREADY_EXISTS`) and `lpCmdLine` excluding the program name | *WinMain (Windows CE 5.0)*, `ms914104` |
| CE `DllMain` prototype `BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)` (handle, not instance); hinstDLL = module base address = HMODULE; initial thread receives only PROCESS_ATTACH; `LoadLibrary` does not notify already-running threads; unload delivers no per-thread DETACH; FALSE on PROCESS_ATTACH ⇒ `LoadLibrary` returns NULL / process-init failure terminates the process; return value ignored for other reasons; no LoadLibrary/FreeLibrary from the entry | *DllMain (Windows CE 5.0)*, `ms885202` |
| `/ENTRY` table: `WinMainCRTStartup` → app calling `__cdecl WinMain`; `wWinMainCRTStartup` → app calling `__cdecl wWinMain`; `_DllMainCRTStartup` → DLL calling `__cdecl DllMain`; with neither `/DLL` nor `/SUBSYSTEM`, the linker picks subsystem and entry from whether main or WinMain is defined; parameters/return must match the documented signatures | */ENTRY (Windows CE 5.0)*, `aa449732` |
| Undecorated, Unicode import model (`coredll.dll` API surface): CE function pages uniformly state “OS Versions: Windows CE 2.0 and later … Link Library: coredll.dll” and that Windows CE supports only the Unicode version of wide APIs | CE run-time function pages (`ms860473` towlower, `ms860377` srand, `ms860374` sprintf, `ms861145` _snprintf, `ms860368` setvbuf, `ms859665` malloc, `ms860384` strcat); *GetCommandLine (Windows CE 3.0)* `ms928607` |
| The CRT's hard coredll imports are available on every target generation (CE 4.x/5.x/6.x): `TerminateProcess`; `GetModuleHandleW` (NULL ⇒ pseudo-handle of the current process); `GetCommandLineW` (the only version Windows CE supports); `GetModuleFileNameW`; `GetProcAddressW`; `LocalAlloc` (CE heap model: local and global heaps are the same); `LocalFree` | Official per-function pages (CE 5.0 archive, all “Link Library: Coredll.lib”): `TerminateProcess` `aa450927` (CE 1.0+); `GetModuleHandle` `ms885630` (CE 2.10+; “Coredll.lib, Nk.lib”); `GetCommandLine` `ms885605` (CE 3.0+; Remarks: “Windows CE supports only the Unicode version of this function”); `GetModuleFileName` `ms885629` (CE 2.0+); `GetProcAddress` `ms885634` (CE 1.0+; Remarks: the ASCII version `GetProcAddressA` is supported for CE 3.0+); `LocalAlloc` `ms886739` (CE 1.0+; documents `LPTR` = `LMEM_FIXED`+`LMEM_ZEROINIT`); `LocalFree` `ms886741` (CE 1.0+) |
| coredll exports `GetProcAddressW` **and** `GetProcAddressA` (no undecorated `GetProcAddress`), so the runtime pins the W spelling which exists on every generation | `ms885634` (above) agrees; the CE 4/5/6 import libraries of the toolchain sysroot define both exports (verified; this corrected a stale source comment that claimed a W-only export) |
| The narrow-argv converter `WideCharToMultiByte(CP_ACP)` is resolved at runtime and degrades (lossy 7-bit) when the OS image lacks it, because optional modules may be cut from an OEM image | *WideCharToMultiByte (Windows CE 3.0)* `ms915519` and *MultiByteToWideChar* `ms961248` carry the official note: “This API is part of the complete Windows CE OS package as provided by Microsoft. … some devices may not support this API” (same note on the CE 3.0-era *TerminateProcess* `ms913239`) |
| Process-exit fallback uses only `TerminateProcess` on the current process (coredll exports no `ExitProcess` on any CE generation — **documented conflict**, see Notes) | `TerminateProcess` `aa450927` (CE 1.0+, Coredll.lib) matches the record; the conflicting page is *ExitProcess (Windows CE 5.0)* `ms885217` (“OS Versions: Windows CE 2.0 and later”, “Link Library: Coredll.lib”), which no CE 4/5/6 import library of the sysroot satisfies, whose role the sysroot's CE headers fill with an inline `TerminateProcess(GetCurrentProcess(), code)` wrapper, and for which no CE 3.0-era page exists in the archive — treated as a documentation error on the export point |

Notes: the CE 5.0 pages above are part of the *Windows CE 5.0
documentation* (MSDN), served as `(v=msdn.10)` archive pages; the
per-function “Requirements” blocks state a minimum CE version (1.0,
2.0, 2.10 or 3.0 depending on the function) plus header and
`coredll.dll` — all minimums lie below the CE 4.x/5.x/6.0 targets,
which is why one record set covers the three generations.  Two
corroborations from the same archive:
`ms859579` states that the CE run-time library supports neither ANSI C
nor POSIX and provides only a Win32-API-compatible subset (no console,
path/filename file handling, locale, time-setting, or process-spawn
routines — matching this CRT's coredll-only import surface), and the
rest of `ms885202` (DllMain restrictions: no `LoadLibrary`/`FreeLibrary`
from the entry, no synchronization inside `DllMain`, serialized entry
calls, and safe Win32 calls during detach limited to TLS, object
creation, and file functions) is guidance to user `DllMain` code that
the implemented `_DllMainCRTStartup` sequence does not conflict with.

The one conflict found: `ms885217` documents `ExitProcess` with
“OS Versions: Windows CE 2.0 and later … Link Library: Coredll.lib”,
but `ExitProcess` is absent from every CE 4/5/6 coredll import
library of the toolchain sysroot (whose defs are cross-checked against
device dumps by the toolchain's `audit-coredll.py`), the sysroot's CE
headers declare `ExitProcess` as an inline wrapper that calls
`TerminateProcess(GetCurrentProcess(), code)` (visible in the compiled
sysroot CRT objects as a `TerminateProcess` call with the
current-process pseudo-handle 66), and no CE 3.0-era `ExitProcess`
page exists in the archive.  The doc page is therefore treated as
inaccurate on that export point, and the verified import surface
governs (an app that referenced `ExitProcess` would not link against
the sysroot `coredll.lib`).  Akari never references `ExitProcess`, so
this conflict changes no code; the stale in-tree claims it exposed —
crt0.c's “ExitProcess documentation covers desktop Windows only” and
runtime.c's “GetProcAddress only in its W spelling” — were corrected
to the statements above.  The CE 6.0-era documentation set
(`(v=winembedded.60)`) that survives on Microsoft Learn holds
Platform Builder and run-time overview material; the per-function API
reference pages for these functions live in the `(v=msdn.10)` archive.
No other contradiction was found with the platform documentation, and
no code change resulted from the re-check.

#### CRT function-coverage audit (no insufficiency found)

The remaining official CRT families were fetched in full and checked
for anything Akari's startup layer must do that it does not:
* *Microsoft C Run-time Library for Windows CE* (`ms861487`) structures
  the CE run-time library into: *C Run-time Libraries* (`ms859579`),
  *Run-time Routines by Category* (`ms859589`: buffer manipulation,
  character classification, data conversion, floating-point support,
  input and output, memory allocation, process control, sorting,
  string manipulation), *Global Variables and Standard Types*
  (`ms859596`), *Run-Time Library Global Constants* (`ms861495`),
  *Generic Text Mappings* (`ms861474`) and the alphabetical
  *Run-time Library Reference* (`ms859613`), plus *Required and
  Optional Headers* (`ms859587`).  All of those describe **library
  (libc) routines**, which this repository does not provide (see the
  responsibility table) — nothing there prescribes behavior for the
  startup layer Akari implements, so there is no missing
  functionality to add.
* *Global Variables (Windows CE 5.0)* (`ms861480`) documents exactly
  one CE run-time global variable: `_fmode` (`ms860503`, “Sets default
  file-translation mode”), which Akari provides.  Akari's remaining
  data globals (`__argc`, `__argv`, `__wargv`, `_acmdln`, `_wcmdln`,
  `_wcmdtail`, `_doserrno`, `_commode`, `__dso_handle`) are not
  documented CE run-time globals; they are the MSVCRT-model names that
  CE-targeting libc/CRT consumers and headers expect, and since coredll
  exports no data objects (verified), the startup layer is their
  natural home — recorded as own design on top of the one documented
  global.
* *GetCommandLine (Windows CE 5.0)* (`ms885605`, fetched in full):
  “Windows CE supports only the Unicode version”, CE 3.0+, Winbase.h,
  Coredll.lib.  No CE page states whether the returned string includes
  the program name or how an empty command line is composed, so the
  parser's argv[0] and empty-command-line handling rests on the
  documented CommandLineToArgvW rules (see “Command-line parsing
  rules”) plus the CE `WinMain` statement that `lpCmdLine` excludes
  the program name — the strongest official sources that exist for
  these points.

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
  `-nostdlib`, so Akari links bypass it) against the **real coredll
  import libraries** of the toolchain sysroot (CE 6.0
  `libcoredll6.a`, CE 5.0 `libcoredll.a`, CE 4.x `libcoredll4.a`,
  x86 CE 6.0 `libcoredll6-x86.a`):
  * EXE (`akari_crt0.o + libakari.a + user`): machine ARM/I386,
    subsystem `WINDOWS_CE_GUI` (9), OS version 6.0, entry resolved to
    the requested CE entry — `mainACRTStartup` (main app),
    `WinMainCRTStartup` (WinMain app), `mainWCRTStartup` (wmain app);
    `.ctors` = `[-1, entries..., 0]`, `.CRT` = the four NULL sentinels
    (plus sorted user entries when an MS-style `.CRT$XU`
    object is present).
  * DLL (`akari_dllcrt.o + libakari.a + user DllMain`,
    `/dll /entry:DllMainCRTStartup`): links; subsystem 9.
  * Entry resolution by `lld-link -wince` (verified): EXE links
    require an explicit `/entry` — with none, lld's subsystem
    default is the desktop `mainCRTStartup`, which CE images
    deliberately do not define.  DLL links resolve without `/entry`
    on ARM (the default search finds the `DllMainCRTStartup` alias);
    on 32-bit x86 the default DLL-entry search uses the desktop
    stdcall-decorated spelling (`__DllMainCRTStartup@12`), so x86 CE
    DLL links must pass `/entry:DllMainCRTStartup`.  On x86, lld
    decorates `/entry` names with a leading underscore — that is why
    Akari provides the underscore aliases, and why `/entry` must be
    spelled without one (`/entry:_DllMainCRTStartup` double-decorates
    and fails).
  * The image imports only the coredll functions the CRT and the app
    actually use (`llvm-readobj --coff-imports`): the CRT's
    `TerminateProcess`, `GetModuleHandleW`, `GetCommandLineW`,
    `GetModuleFileNameW`, `GetProcAddressW`, `LocalAlloc`,
    `LocalFree` — no desktop-API imports.
* **coredll import surface** (verified against the CE 4/5/6 import
  libraries of the sysroot): coredll exports no `ExitProcess` and no
  undecorated `GetProcAddress` on any CE generation — process
  termination is `TerminateProcess`, and the export is
  `GetProcAddressW`/`GetProcAddressA` (SDK headers map
  `GetProcAddress` to the W form).  coredll export names are also
  **undecorated on x86** (the import libraries define
  `__imp_<name>` without the x86 leading underscore), so Akari pins
  every coredll import declaration with an asm label to the
  undecorated spelling and references the `__imp_` slot through
  `__declspec(dllimport)` — verified on x86 objects (`__imp_` +
  undecorated name relocations).  The sysroot's own CE CRT objects
  were inspected for comparison: they implement `ExitProcess` as a
  local wrapper that calls `TerminateProcess` with the current-process
  pseudo-handle 66 (`SH_CURPROC` 2 + `SYS_HANDLE_BASE` 64 in the CE
  system-handle space; observed as `mov r0, #66` in the compiled
  objects).  Akari follows the same CE-native surface: `os_exit`
  calls `TerminateProcess(AKARI_CURRENT_PROCESS, rc)` and
  `resolve_w2m` uses `GetProcAddressW`.
* x86 CE end-to-end: the toolchain sysroot's `libcoredll6-x86.a`
  contains ARM (armce) import objects (verified — it cannot be used
  in an x86 link), so the x86 link check was done with an x86 import
  library generated from the same `coredll6-x86.def` by
  `llvm-dlltool -m i386 --no-leading-underscore`; Akari objects link
  cleanly against it (machine I386, subsystem 9, undecorated import
  table).  Apps compiled against the sysroot's mingwrt headers still
  reference decorated x86 names (`_GetTickCount`), which that
  undecorated import library does not satisfy — a sysroot/header
  concern for x86 CE, outside Akari.
* `lld-link` rejects versioned `/subsystem:windowsce:5.02` spellings;
  bare `/subsystem:windowsce` stamps version 6.0 (verified).
* `___main`: i686 `windows-gnu` clang injects a call into `main()`
  (so Akari keeps its no-op `___main`); `i386-pc-wince` does not
  (verified on objects).
* clang-cl for `*-pc-wince` (both i386 and ARM) emits GNU-style
  `.ctors` entries for C++ static initializers (`_GLOBAL__sub_I_*`)
  and registers destructors via `atexit` — not `.CRT$XCU` (verified
  on objects).  `.CRT$XCU`/`.CRT$XIU` appear only when user code
  allocates them (`__declspec(allocate(...))`, MS-style objects); the
  sentinel brackets around them were verified in both object orders
  on both architectures (see "Constructor/destructor lists").
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
* Per-CE-version export coverage of the imported APIs against the
  official per-function documentation was audited (see the
  “Official-source cross-check” section): each import is documented
  for CE 4.x–6.x with “Link Library: Coredll.lib” and matches the
  sysroot import surface, with the single documented `ExitProcess`
  conflict described there.  Images that cut optional modules are
  handled defensively at runtime (see the `WideCharToMultiByte`
  fallback above).

## License

MIT — see the notice at the top of each file and the LICENSE file.

© 2026 Akari CRT contributors.
