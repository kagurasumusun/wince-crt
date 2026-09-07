# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Build products and their roles:
#
#   build/akari_crt0.o   (from src/crt/crt0.c)
#       EXE startup object.  Contains the Windows CE EXE entry points
#       WinMainCRTStartup, wWinMainCRTStartup, mainACRTStartup and
#       mainWCRTStartup, plus the underscore-prefixed aliases used by
#       the older CE x86 tools.  The linker selects the entry point
#       with --entry=<name> (or /ENTRY:<name>); unreferenced entries
#       are dropped by --gc-sections, so they cost nothing at runtime.
#
#       Note: Windows CE has no mainCRTStartup -- Microsoft's CE
#       documentation assigns main() programs to mainACRTStartup (see
#       "Linking to the CRT" and "/ENTRY", Windows CE 5.0).  That name
#       is deliberately not defined.
#
#   build/akari_crt0w.o, build/akari_crt0c.o
#       Byte-identical copies of akari_crt0.o kept for compatibility
#       with older link lines that named the w/c startup objects.
#
#   build/akari_dllcrt.o (from src/crt/dllcrt.c)
#       DLL startup object.  Contains the canonical CE spelling
#       _DllMainCRTStartup and the unadorned DllMainCRTStartup alias
#       that lld's default DLL-entry search tries first, plus a weak
#       default DllMain.
#
#   build/libakari.a     (from src/crt/runtime.c)
#       Per-module runtime support that BOTH products need: the
#       MSVCRT-model data globals (__argc/__argv/__wargv/_acmdln/
#       _wcmdln/_wcmdtail/_fmode/_doserrno/_commode/__dso_handle), the
#       command-line parser and narrow-argv synthesis, the global
#       constructor/destructor runners, and the x86 ___main hook.
#
# SCOPE: startup/process glue ONLY, for Windows CE 4.0-6.0 images
# built with Clang/lld (windows-gnu PE/COFF).  No C library, no C++
# runtime, no libc replacement is implemented here -- see README for
# the responsibility table.  All CPU-specific lowering is clang's;
# PE/COFF layout and subsystem selection are lld's.
#
# Cross-build usage:
#   make TARGET=armv7-unknown-windows-gnu            # ARMv7 (CE 6 class)
#   make TARGET=armv7-unknown-windows-gnu ARCHFLAGS=-mthumb   # Thumb2
#   make TARGET=armv5-unknown-windows-gnu            # ARMv5 (CE 5 class)
#   make TARGET=i686-unknown-windows-gnu             # x86 (CEPC etc.)
#   make TARGET=<triple> CC=/path/to/clang AR=/path/to/llvm-ar
#
# Host-side checks (no cross toolchain needed):
#   make hostcheck   # compile every TU warning-free with the host cc
#   make hosttest    # build and RUN the host parser self-test
#   make check       # hostcheck + hosttest

TARGET        ?= armv7-unknown-windows-gnu
CC            ?= clang
AR            ?= llvm-ar
ARCHFLAGS     ?=

INCLUDES      = -Iinclude

# Target flags.
#   -D_AKARI_BUILD=1, -D_WIN32_WCE=0x0500 : Akari sources are CE-only
#       and must never see desktop-Win32 preprocessor behavior.
#   -fno-builtin       : we are the layer below any hosted runtime.
#   -nostdlibinc       : no host/desktop libc headers.
#   -fno-stack-protector: no __stack_chk_guard exists yet at entry.
#   (NO -ffreestanding: Clang 22.x ARM windows-gnu emits invalid .seh
#   sequences for -ffreestanding code at -Os; verified toolchain
#   behavior, see README.)
TARGET_FLAGS  = -D_AKARI_BUILD=1 -D_WIN32_WCE=0x0500 \
                -fno-builtin -nostdlibinc -fno-stack-protector

CFLAGS        = -Os -ffunction-sections -fdata-sections \
                -Wall -Wextra -Wshadow -Wstrict-prototypes \
                -Wmissing-prototypes \
                $(ARCHFLAGS) $(INCLUDES) $(TARGET_FLAGS)

BUILD         = build
C_SRCS        = src/crt/crt0.c src/crt/dllcrt.c src/crt/runtime.c
HDRS          = include/akari/compiler.h include/akari/crt.h \
                include/akari/internal.h

CRT0_OBJ      = $(BUILD)/akari_crt0.o
CRT0W_OBJ     = $(BUILD)/akari_crt0w.o
CRT0C_OBJ     = $(BUILD)/akari_crt0c.o
DLLCRT_OBJ    = $(BUILD)/akari_dllcrt.o
LIB           = $(BUILD)/libakari.a

.PHONY: all clean install check hostcheck hosttest

all: $(LIB) $(CRT0_OBJ) $(DLLCRT_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ)

$(BUILD):
	@mkdir -p $(BUILD)

# Rule for the three intermediate objects (kept in build/, never in
# the source tree).
$(BUILD)/%.o: src/crt/%.c $(HDRS) | $(BUILD)
	$(CC) --target=$(TARGET) $(CFLAGS) -c $< -o $@

$(LIB): $(BUILD)/runtime.o
	$(AR) crs $@ $<

$(CRT0_OBJ): $(BUILD)/crt0.o
	cp $(BUILD)/crt0.o $(CRT0_OBJ)

# Backward-compatibility aliases (byte-identical copies).
$(CRT0W_OBJ): $(CRT0_OBJ)
	cp $(CRT0_OBJ) $(CRT0W_OBJ)
$(CRT0C_OBJ): $(CRT0_OBJ)
	cp $(CRT0_OBJ) $(CRT0C_OBJ)

$(DLLCRT_OBJ): $(BUILD)/dllcrt.o
	cp $(BUILD)/dllcrt.o $(DLLCRT_OBJ)

install: all
	install -d $(PREFIX)/lib $(PREFIX)/include/akari
	install -m 644 $(LIB)        $(PREFIX)/lib/
	install -m 644 $(CRT0_OBJ)   $(PREFIX)/lib/
	install -m 644 $(CRT0W_OBJ)  $(PREFIX)/lib/
	install -m 644 $(CRT0C_OBJ)  $(PREFIX)/lib/
	install -m 644 $(DLLCRT_OBJ) $(PREFIX)/lib/
	install -m 644 include/akari/compiler.h $(PREFIX)/include/akari/
	install -m 644 include/akari/crt.h      $(PREFIX)/include/akari/

clean:
	rm -rf $(BUILD)

# ---- Host-side checks -------------------------------------------------
# hostcheck: compile every TU with the host cc, full warnings, and
# archive.  Catches syntax/portability mistakes without a cross
# toolchain.  (On the host, _WIN32/_WIN64 are undefined, so the
# asm-label entry pinning, dllimport and x86-only code compile out;
# the host build is a compile/parse check only.)
HOSTCC      ?= cc
HOSTCFLAGS   = -std=gnu11 -Os -Wall -Wextra -Wshadow \
               -Wstrict-prototypes -Wmissing-prototypes \
               -D_AKARI_BUILD=1 -Iinclude

HOST_OBJDIR  = $(BUILD)/host

hostcheck: | $(BUILD)
	@echo "[hostcheck] compiling $(C_SRCS) with $(HOSTCC)"
	@mkdir -p $(HOST_OBJDIR)
	@set -e; for f in $(C_SRCS); do \
	    echo "  CC  $$f"; \
	    $(HOSTCC) $(HOSTCFLAGS) -c "$$f" -o "$(HOST_OBJDIR)/$$(basename $$f .c).o"; \
	done
	@$(AR) crs $(HOST_OBJDIR)/libakari.a \
	    $(C_SRCS:src/crt/%.c=$(HOST_OBJDIR)/%.o)
	@echo "[hostcheck] OK -- all sources compile and archive warning-free"

# hosttest: the runtime command-line machinery, exercised on the host
# against the documented parsing rules (tests/host/test_main.c
# provides coredll stubs).  Prints "all N checks passed" on success.
# (-Wmissing/strict-prototypes are relaxed for the test TU: its
# coredll stubs intentionally repeat runtime.c's own declarations.)
hosttest: | $(BUILD)
	@echo "[hosttest] building and running the parser self-test"
	@mkdir -p $(HOST_OBJDIR)
	$(HOSTCC) $(HOSTCFLAGS) -Wno-missing-prototypes \
	    -Wno-strict-prototypes tests/host/test_main.c \
	    src/crt/runtime.c -o $(HOST_OBJDIR)/hosttest
	@$(HOST_OBJDIR)/hosttest

check: hostcheck hosttest
