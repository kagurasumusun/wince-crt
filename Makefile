# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Build products and their roles:
#
#   akari_crt0.o     (built from src/crt/crt0.c)
#       Single EXE-startup object.  It contains ALL five PE entry
#       points (WinMainCRTStartup, wWinMainCRTStartup, mainCRTStartup,
#       mainWCRTStartup, mainACRTStartup).  Link this ONCE (and only
#       once) into any EXE.  The linker selects which entry point to
#       use from the -Wl,-entry:<name> (or /ENTRY:<name>) flag; unused
#       entry symbols are dropped by --gc-sections, so they cost
#       nothing at runtime.
#
#       We deliberately ship ONE object (not three) because the
#       entry-point selection is the linker's job, and shipping
#       akari_crt0.o / akari_crt0w.o / akari_crt0c.o as three
#       separately-compiled copies of the same TU created redundant
#       build work, redundant install footprints, and risked the
#       consumer accidentally linking more than one startup object.
#       For backward compatibility we also install compatibility
#       aliases (akari_crt0w.o, akari_crt0c.o) that are exact copies
#       of akari_crt0.o; older link scripts that named them
#       explicitly continue to work.
#
#   akari_dllcrt.o   (built from src/crt/dllcrt.c)
#       Single DLL-startup object containing _DllMainCRTStartup.
#       Link this once into any DLL that does not provide its own
#       DllMainCRTStartup.
#
#   libakari.a       (contains: MSVCRT data globals + constructor
#       dispatch helpers + weak default WinMain/DllMain fallbacks)
#       Convenience static library that any consumer links in,
#       regardless of whether they build an EXE or a DLL.  It does
#       NOT contain the PE entry points themselves -- those live in
#       the separate akari_crt0.o / akari_dllcrt.o objects, since
#       linking an archive does not automatically pull in an
#       unreferenced entry point unless the linker is told to do so
#       via --undefined / -u.  Splitting entry points out into their
#       own object is the standard layout used by every CRT (msvcrt,
#       mingwrt, glibc crt1.o, etc.).
#
# SCOPE: Akari is a minimal CRT startup / ABI-glue layer ONLY for
# Windows CE 4.0 through 6.0 on the architectures CE supports:
#   ARM (v4 / v4i / v5 / v6 / v7 IWMMXT, in ARM or Thumb state),
#   x86 (i486 and later),
#   MIPS (MIPSII / MIPSII_FP / MIPSIV / MIPSIV_FP / MIPS16),
#   SuperH (SH3 / SH4).
#
# Akari is architecture-neutral: no source file contains inline
# assembly, endianness assumptions, or calling-convention
# decorators specific to a single ISA.  All CPU-specific lowering is
# the job of clang/LLVM, and PE/COFF layout / subsystem selection is
# the job of lld.
#
# Provided by other components (NOT shipped by Akari):
#   - C library (stdio/stdlib/string/math/exit/atexit/setjmp/longjmp/
#     __stack_chk_guard/__stack_chk_fail/...) : libc (coredll.dll
#     msvcrt exports / newlib / llvm-libc).
#   - C++ runtime / exceptions / RTTI : libc++ / libc++abi / llvm-libunwind.
#   - Compiler builtins (__chkstk / __aeabi_*) : compiler-rt (linked
#     automatically by clang).
#   - TLS callbacks (__tls_used / .tls directory) : the linker (lld)
#     synthesises the TLS directory from __declspec(thread) objects;
#     the OS calls TLS callbacks before DllMain.
#   - Linker section layout, subsystem selection, entry-point
#     resolution : lld.  Pass -Wl,-subsystem:windowsce:<ver> on the
#     link line (use :4.0 for CE 4, :5.0 / :5.01 / :5.02 for CE 5,
#     :9.0 for CE 6.x / Windows Mobile 6.x).
#   - Win32 SDK headers / coredll import library : consumer provides.
#
# Akari's job: PE entry -> GetCommandLineW -> __argc/__argv/__wargv
# -> .init_array/.ctors -> user WinMain/main -> libc exit() ->
# atexit/__cxa_atexit destructors (via libc) -> ExitProcess (via libc).

# Cross-compiler prefix.  Override on the make command line to
# switch target architectures, e.g.:
#   make CROSS=armv4-wince-         # ARMv4   (Windows Mobile 2003 class)
#   make CROSS=armv5-wince-         # ARMv5   (CE 5 / WM5 class)
#   make CROSS=armv7-wince-         # ARMv7   (CE 6 / WM6.5 class, Thumb2)
#   make CROSS=i686-wince-          # x86     (CE PC / CEPC / x86 emulator)
#   make CROSS=mips-wince-          # MIPS
#   make CROSS=sh4-wince-           # SuperH 4
#
# The triple suffix "-wince-" selects windows-gnu (MinGW) PE/COFF
# output through the clang driver; lld emits the subsystem:windowsce
# header when you pass -Wl,-subsystem:windowsce:<ver>.
CROSS       ?= armv7-wince-
CC          = $(CROSS)clang
AR          = $(CROSS)llvm-ar

INCLUDES    = -Iinclude
# Target flags:
#   -ffreestanding / -fno-builtin : no hosted-C assumptions; we ARE
#       the startup layer.
#   -nostdlibinc : do not pull host libc headers; Win32 types are
#       forward-declared locally in the CRT sources.
#   -fno-stack-protector : the CRT runs before __stack_chk_guard is
#       initialised (stack-chk fail is provided by libc).
TARGET_FLAGS = -ffreestanding -fno-builtin -nostdlibinc \
               -fno-stack-protector -D_AKARI_BUILD=1
CFLAGS      = -Os -fvisibility=hidden \
              -Wall -Wextra -Wshadow -Wstrict-prototypes \
              -Wmissing-prototypes -Wno-long-long \
              -ffunction-sections -fdata-sections \
              $(INCLUDES) $(TARGET_FLAGS)
ARFLAGS     = cr

# Sources: two translation units.
C_SRCS      = src/crt/crt0.c src/crt/dllcrt.c
# Intermediate objects used only for building libakari.a; these go
# into src/crt/*.o (out-of-source would be nicer, but kept simple).
C_OBJS      = $(C_SRCS:.c=.o)

# Delivered artifacts.
CRT0_OBJ    = build/akari_crt0.o
CRT0W_OBJ   = build/akari_crt0w.o
CRT0C_OBJ   = build/akari_crt0c.o
DLLCRT_OBJ  = build/akari_dllcrt.o
LIB         = build/libakari.a

.PHONY: all clean install hostcheck

all: $(LIB) $(CRT0_OBJ) $(DLLCRT_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ)

build:
	@mkdir -p build

$(C_OBJS): %.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# libakari.a contains the MSVCRT data globals, the constructor/
# destructor runners, and the weak default WinMain/DllMain fallbacks.
# It deliberately does NOT contain the PE entry points -- those are
# in the separate akari_crt0.o / akari_dllcrt.o.
$(LIB): $(C_OBJS) | build
	$(AR) $(ARFLAGS) $@ $(C_OBJS)

# akari_crt0.o is the single EXE startup object.  We compile it with
# the same flags as the lib objects; it carries all five entry
# points.
$(CRT0_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# Backward-compatibility aliases (byte-identical copies): older
# scripts that reference akari_crt0w.o or akari_crt0c.o by name
# continue to work.
$(CRT0W_OBJ): $(CRT0_OBJ)
	cp $(CRT0_OBJ) $(CRT0W_OBJ)
$(CRT0C_OBJ): $(CRT0_OBJ)
	cp $(CRT0_OBJ) $(CRT0C_OBJ)

$(DLLCRT_OBJ): src/crt/dllcrt.c | build
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(PREFIX)/lib $(PREFIX)/include/akari
	install -m 644 $(LIB)        $(PREFIX)/lib/
	install -m 644 $(CRT0_OBJ)   $(PREFIX)/lib/
	install -m 644 $(CRT0W_OBJ)  $(PREFIX)/lib/
	install -m 644 $(CRT0C_OBJ)  $(PREFIX)/lib/
	install -m 644 $(DLLCRT_OBJ) $(PREFIX)/lib/
	cp include/akari/*.h $(PREFIX)/include/akari/

clean:
	rm -rf build $(C_OBJS)

# ---- Host-side build check: compile every TU with the host gcc
#      warning-free and archive.  This catches syntax/portability
#      mistakes early without an ARM/x86/MIPS/SH cross toolchain.
HOSTCC      ?= gcc
HOSTCFLAGS  = -Os -Wall -Wextra -std=c99 -ffreestanding \
              -D_AKARI_BUILD=1 -D_DEBUG_HOSTCHECK_ \
              -Iinclude -Iinclude/akari \
              -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
              -Wno-long-long

hostcheck: | build
	@echo "[hostcheck] building CRT objects with $(HOSTCC)"
	@mkdir -p build/host
	@set -e; for f in $(C_SRCS); do \
	    echo "  CC  $$f"; \
	    mkdir -p "build/host/$$(dirname $$f)"; \
	    $(HOSTCC) $(HOSTCFLAGS) -c "$$f" -o "build/host/$${f%.c}.o"; \
	done
	@echo "[hostcheck] archiving into build/host/libakari.a"
	@ar crs build/host/libakari.a $(C_SRCS:%.c=build/host/%.o)
	@echo "[hostcheck] OK -- all sources compile and archive warning-free"
