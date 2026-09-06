# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Usage:
#   make CROSS=armv7-wince-
#       or, for host-side syntax checking:
#   make hostcheck
#
# This Makefile builds:
#   - libakari.a    static C runtime
#   - akari_crt0.o  EXE startup (WinMainCRTStartup)
#   - akari_crt0w.o EXE startup (wWinMainCRTStartup)
#   - akari_crt0c.o EXE startup (mainCRTStartup)
#   - akari_dllcrt.o DLL startup (DllMainCRTStartup)
#   - import libs    coredll import library (via .def file)

# ---- Configuration ----
CROSS       ?= arm-wince-mingw32ce-
CC          = $(CROSS)clang
AR          = $(CROSS)llvm-ar
AS          = $(CROSS)clang
LD          = $(CROSS)ld.lld

SYSROOT     ?=
INCLUDES    = -Iinclude -Iinclude/akari
TARGET_FLAGS = -target armv7-unknown-windows-gnu -fshort-wchar -mthumb -D_AKARI_WANT_SHORT_WCHAR=1
CFLAGS      = -Os -fno-builtin -ffreestanding -fno-stack-protector \
              -fvisibility=hidden -Wall -Wextra -D_AKARI_BUILD=1 \
              $(INCLUDES) $(TARGET_FLAGS)
ASFLAGS     = $(CFLAGS)
ARFLAGS     = cr

# ---- Source files ----
C_SRCS      = \
    src/ctype/ctype.c \
    src/string/mem.c \
    src/string/str.c \
    src/string/wcs.c \
    src/string/wcscollate.c \
    src/string/strextra.c \
    src/stdlib/malloc.c \
    src/stdlib/strtol.c \
    src/stdlib/strtoll.c \
    src/stdlib/wcsto.c \
    src/stdlib/qsort.c \
    src/stdlib/bsearch.c \
    src/stdlib/rand.c \
    src/stdlib/abs.c \
    src/stdlib/env.c \
    src/stdlib/search.c \
    src/stdlib/itoa.c \
    src/stdlib/itoa_w.c \
    src/stdlib/stdlib_extra.c \
    src/misc/atexit.c \
    src/misc/errno.c \
    src/misc/exit.c \
    src/misc/mbstring.c \
    src/misc/assert.c \
    src/misc/ansishim.c \
    src/misc/locale.c \
    src/misc/signal.c \
    src/misc/globals.c \
    src/misc/msvcrt_stubs.c \
    src/stdio/printf.c \
    src/stdio/snprintf.c \
    src/stdio/scanf.c \
    src/stdio/wprintf.c \
    src/stdio/stdio.c \
    src/stdio/stdio_stubs.c \
    src/stdio/posix_io.c \
    src/stdio/wscanf.c \
    src/stdio/conio.c \
    src/math/math.c \
    src/time/time.c \
    src/misc/hoststubs.c \
    src/crt/crt_cpp.c \
    src/crt/patchables.c \
    src/compiler-rt/udivmodsi4.c

ARM_ASM_SRCS = \
    src/setjmp/setjmp_arm.S \
    src/compiler-rt/chkstk_arm.S \
    src/compiler-rt/aeabi_idivmod.S
X86_ASM_SRCS = \
    src/setjmp/setjmp_x86.S \
    src/compiler-rt/chkstk_x86.S
MIPS_ASM_SRCS = \
    src/setjmp/setjmp_mips.S \
    src/compiler-rt/chkstk_mips.S
SH_ASM_SRCS   = \
    src/setjmp/setjmp_sh.S \
    src/compiler-rt/chkstk_sh.S

STARTUP_C_SRCS = src/crt/crt0.c src/crt/dllcrt.c

C_OBJS      = $(C_SRCS:.c=.o)
ARM_ASM_OBJS= $(ARM_ASM_SRCS:.S=.o)

LIB_OBJS    = $(C_OBJS) $(ARM_ASM_OBJS)

CRT0_OBJ    = build/akari_crt0.o
CRT0W_OBJ   = build/akari_crt0w.o
CRT0C_OBJ   = build/akari_crt0c.o
DLLCRT_OBJ  = build/akari_dllcrt.o
LIB         = build/libakari.a

.PHONY: all clean install hostcheck test

all: $(LIB) $(CRT0_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ) $(DLLCRT_OBJ) \
     build/coredll.lib

build:
	@mkdir -p build src/ctype src/string src/stdlib src/misc src/stdio \
	    src/math src/time src/crt src/compiler-rt src/setjmp

$(C_OBJS): %.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARM_ASM_OBJS): %.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(LIB): $(LIB_OBJS) | build
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

# Startup objects are compiled from the same source file with different defines
$(CRT0_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -D_AKARI_ENTRY=WinMainCRTStartup -c $< -o $@
$(CRT0W_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -D_AKARI_ENTRY=wWinMainCRTStartup -c $< -o $@
$(CRT0C_OBJ): src/crt/crt0.c | build
	$(CC) $(CFLAGS) -D_AKARI_ENTRY=mainCRTStartup -c $< -o $@
$(DLLCRT_OBJ): src/crt/dllcrt.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/coredll.lib: src/coredll.def | build
	$(CROSS)llvm-dlltool -d $< -l $@ -m arm

install: all
	install -d $(PREFIX)/lib $(PREFIX)/include $(PREFIX)/lib/ldscripts
	install -m 644 $(LIB) $(PREFIX)/lib/
	install -m 644 $(CRT0_OBJ) $(PREFIX)/lib/
	install -m 644 $(CRT0W_OBJ) $(PREFIX)/lib/
	install -m 644 $(CRT0C_OBJ) $(PREFIX)/lib/
	install -m 644 $(DLLCRT_OBJ) $(PREFIX)/lib/
	install -m 644 build/coredll.lib $(PREFIX)/lib/
	cp -R include/* $(PREFIX)/include/
	cp ldscripts/*.ld $(PREFIX)/lib/ldscripts/

clean:
	rm -rf build $(C_OBJS) $(ARM_ASM_OBJS)

# ---- Host-side syntax check ----
# Uses the native host compiler to check that portable C sources compile.
HOSTCC      ?= gcc
HOSTCFLAGS  = -Os -Wall -Wextra -std=c99 -ffreestanding -fshort-wchar \
              -D_AKARI_BUILD=1 -D_AKARI_WANT_SHORT_WCHAR=1 \
              -Iinclude -Iinclude/akari -D_DEBUG_HOSTCHECK_ \
              -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
              -Wno-incompatible-pointer-types -Wno-builtin-declaration-mismatch \
              -Wno-long-long -Wno-overflow -Wno-pointer-sign
HOST_SRCS   = $(filter-out src/compiler-rt/% src/crt/patchables.c,$(C_SRCS))
hostcheck:
	@echo "[hostcheck] compiling portable sources with $(HOSTCC)"
	@set -e; for f in $(HOST_SRCS); do \
	    echo "  CC  $$f"; \
	    $(HOSTCC) $(HOSTCFLAGS) -c $$f -o /dev/null; \
	done
	@echo "[hostcheck] OK"

# ---- Host-side functional test ----
# Links the portable CRT sources against the host-check backends (raw Linux
# syscalls) and runs a small sanity suite. Intended for smoke-testing logic
# before a cross build.
HOSTTEST_CFLAGS = -g -O0 -fshort-wchar -D_DEBUG_HOSTCHECK_ \
                  -D_AKARI_WANT_SHORT_WCHAR=1 -D_AKARI_BUILD=1 -std=c99 \
                  -nostdinc -I/usr/lib/gcc/x86_64-linux-gnu/12/include \
                  $(INCLUDES) -ffreestanding \
                  -Wno-pointer-sign -Wno-incompatible-pointer-types \
                  -Wno-builtin-declaration-mismatch -Wno-unused-parameter \
                  -Wno-unused-variable -Wno-long-long -Wno-shadow \
                  -Wno-implicit-function-declaration -Wno-return-type
HOSTTEST_SRCS = tests/test_basic.c $(filter-out src/compiler-rt/% src/crt/patchables.c,$(C_SRCS))
test: | build
	@echo "[test] building host self-test"
	@$(HOSTCC) $(HOSTTEST_CFLAGS) -o build/akari_selftest $(HOSTTEST_SRCS) \
	    -z noexecstack
	@echo "[test] running"
	@./build/akari_selftest
	@echo "[test] OK"
