# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Usage:
#   make CROSS=armv7-wince-
#   make hostcheck       (compile every TU with host gcc to validate build)
#
# This Makefile builds:
#   libakari.a      CRT static glue library (startup + ABI helpers)
#   akari_crt0.o    EXE startup (WinMainCRTStartup)
#   akari_crt0w.o   EXE startup (wWinMainCRTStartup)
#   akari_crt0c.o   EXE startup (mainCRTStartup)
#   akari_dllcrt.o  DLL startup (_DllMainCRTStartup)
#
# SCOPE NOTE: Akari is a CRT/startup/ABI glue layer only.  It does NOT
# ship a C library (use llvm-libc / newlib / coredll msvcrt exports) or
# a Win32 SDK.  Compiler builtins (__chkstk, __aeabi_*, ...) come from
# compiler-rt which clang links automatically; a linker script is not
# required -- pass -Wl,-subsystem:windowsce:9.0
# -Wl,-entry:WinMainCRTStartup to lld.

CROSS       ?= armv7-wince-
CC          = $(CROSS)clang
AR          = $(CROSS)llvm-ar

INCLUDES    = -Iinclude
TARGET_FLAGS = -target thumbv7-unknown-windows-gnu -fshort-wchar -mthumb \
              -ffreestanding -fno-builtin -nostdlibinc \
              -D_AKARI_BUILD=1
CFLAGS      = -Os -fvisibility=hidden -Wall -Wextra $(INCLUDES) $(TARGET_FLAGS)
ARFLAGS     = cr

# ---- Source files (CRT glue only -- no libc, no SDK, no compiler-rt) ----
C_SRCS = \
    src/crt/crt0.c \
    src/crt/crt_cpp.c \
    src/crt/dllcrt.c \
    src/crt/patchables.c \
    src/misc/atexit.c \
    src/misc/errno.c \
    src/misc/exit.c \
    src/misc/globals.c

C_OBJS   = $(C_SRCS:.c=.o)
LIB_OBJS = $(C_OBJS)

CRT0_OBJ   = build/akari_crt0.o
CRT0W_OBJ  = build/akari_crt0w.o
CRT0C_OBJ  = build/akari_crt0c.o
DLLCRT_OBJ = build/akari_dllcrt.o
LIB        = build/libakari.a

.PHONY: all clean install hostcheck

all: $(LIB) $(CRT0_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ) $(DLLCRT_OBJ)

build:
	@mkdir -p build src/crt src/misc

$(C_OBJS): %.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(LIB_OBJS) | build
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

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
	install -m 644 $(LIB) $(PREFIX)/lib/
	install -m 644 $(CRT0_OBJ) $(PREFIX)/lib/
	install -m 644 $(CRT0W_OBJ) $(PREFIX)/lib/
	install -m 644 $(CRT0C_OBJ) $(PREFIX)/lib/
	install -m 644 $(DLLCRT_OBJ) $(PREFIX)/lib/
	cp include/akari/*.h $(PREFIX)/include/akari/

clean:
	rm -rf build $(C_OBJS)

# ---- Host-side build check: compile every TU with gcc (warning-free)
#      and archive, to catch syntax/type errors without an ARM cross
#      toolchain.
HOSTCC      ?= gcc
HOSTCFLAGS  = -Os -Wall -Wextra -std=c99 -ffreestanding -fshort-wchar \
              -D_AKARI_BUILD=1 -D_DEBUG_HOSTCHECK_ \
              -Iinclude -Iinclude/akari \
              -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
              -Wno-long-long -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
              -Wno-incompatible-pointer-types -Wno-builtin-declaration-mismatch \
              -Wno-pedantic

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
	@echo "[hostcheck] OK -- all sources compile and archive"
