# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Builds:
#   libakari.a       static CRT glue library
#   akari_crt0.o     contains WinMainCRTStartup  (+mainACRTStartup alias)
#   akari_crt0w.o    contains wWinMainCRTStartup (+mainWCRTStartup alias)
#   akari_crt0c.o    contains mainCRTStartup
#   akari_dllcrt.o   contains _DllMainCRTStartup
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
#   - C library (stdio/stdlib/string/math/...) : libc
#       (coredll.dll msvcrt exports / newlib / llvm-libc).
#   - C++ runtime / exceptions / RTTI          : libc++ / libc++abi.
#   - Compiler builtins (__chkstk / __aeabi_*): compiler-rt
#       (linked automatically by clang).
#   - Linker section layout                    : lld.  Pass
#       -Wl,-subsystem:windowsce:9.0 -Wl,-entry:<...Startup> on the
#       link line (for CE 6 / WM6; use :4.0 / :5.0 as needed).
#   - Win32 SDK headers / coredll import lib   : consumer provides.
#
# Akari's job: PE entry -> GetCommandLineW -> __argc/__argv/__wargv
# -> .init_array/.ctors -> user WinMain/main -> libc exit() -> atexit
# / __cxa_atexit destructors -> ExitProcess.

# Cross-compiler prefix.  Override on the make command line to
# switch target architectures, e.g.:
#   make CROSS=armv4-wince-         # ARMv4   (Windows Mobile 2003 class)
#   make CROSS=armv5-wince-         # ARMv5   (CE 5 / WM5 class)
#   make CROSS=armv7-wince-         # ARMv7   (CE 6 / WM6.5 class, Thumb2)
#   make CROSS=i686-wince-          # x86     (CE PC / CEPC / x86 emulator)
#   make CROSS=mips-wince-          # MIPS
#   make CROSS=sh4-wince-           # SuperH 4
#
# The triple suffix "-wince-" selects windows-gnu (MinGW) output
# through the clang driver; lld produces a PE/COFF .exe/.dll with
# subsystem:windowsce when you pass -Wl,-subsystem:windowsce:<ver>.
CROSS       ?= armv7-wince-
CC          = $(CROSS)clang
AR          = $(CROSS)llvm-ar

INCLUDES    = -Iinclude
# -fno-short-wchar: on Windows (including CE) wchar_t is the native
# 16-bit wide-character type; we do NOT use the GCC-style -fshort-wchar
# mode (which turns wchar_t into unsigned short) because clang's
# windows-gnu driver already defines _WCHAR_T to match the MS ABI.
# -ffreestanding: no hosted assumptions; we are building the CRT.
# -nostdlibinc: do NOT pull in the host C library headers; Win32
# types are forward-declared locally.
TARGET_FLAGS = -ffreestanding -fno-builtin -nostdlibinc \
               -fno-stack-protector -D_AKARI_BUILD=1
CFLAGS      = -Os -fvisibility=hidden \
              -Wall -Wextra -Wshadow -Wstrict-prototypes \
              -Wmissing-prototypes -Wno-long-long \
              $(INCLUDES) $(TARGET_FLAGS)
ARFLAGS     = cr

# Sources: EXE startup + DLL startup only (2 translation units).
C_SRCS      = src/crt/crt0.c src/crt/dllcrt.c
C_OBJS      = $(C_SRCS:.c=.o)

CRT0_OBJ    = build/akari_crt0.o
CRT0W_OBJ   = build/akari_crt0w.o
CRT0C_OBJ   = build/akari_crt0c.o
DLLCRT_OBJ  = build/akari_dllcrt.o
LIB         = build/libakari.a

.PHONY: all clean install hostcheck

all: $(LIB) $(CRT0_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ) $(DLLCRT_OBJ)

build:
	@mkdir -p build

$(C_OBJS): %.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(C_OBJS) | build
	$(AR) $(ARFLAGS) $@ $(C_OBJS)

# The four startup objects are compiled individually so that a
# consumer can link exactly one of them (whichever entry point they
# want to expose) without dragging in the others.
$(CRT0_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -c $< -o $@
$(CRT0W_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -c $< -o $@
$(CRT0C_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -c $< -o $@
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
