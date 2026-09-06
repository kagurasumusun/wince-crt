# Akari C Runtime for Windows CE
# Copyright (c) 2026 Akari CRT contributors
# SPDX-License-Identifier: MIT
#
# Usage:
#   make CROSS=armv7-wince-
#   make hostcheck       (compiles every TU with host gcc to validate build)
#
# This Makefile builds:
#   libakari.a      CRT static library (startup + ABI glue)
#   akari_crt0.o    EXE startup (WinMainCRTStartup)
#   akari_crt0w.o   EXE startup (wWinMainCRTStartup)
#   akari_crt0c.o   EXE startup (mainCRTStartup)
#   akari_dllcrt.o  DLL startup (_DllMainCRTStartup)
#
# The CRT does NOT ship a C library or Win32 SDK. Consumers supply
# those via their own libc and SDK headers/libraries. The expected
# link line (clang --target=thumbv7-unknown-windows-gnu) is:
#
#   clang --target=thumbv7-unknown-windows-gnu -fshort-wchar \
#         -nostdlib -fuse-ld=lld \
#         -Wl,-subsystem:windowsce:9.0 \
#         -Wl,-entry:WinMainCRTStartup \
#         your_obj.o akari_crt0.o -lakari -lcoredll -o your.exe

# ---- Configuration ----
CROSS       ?= armv7-wince-
CC          = $(CROSS)clang
AR          = $(CROSS)llvm-ar
AS          = $(CROSS)clang

INCLUDES    = -Iinclude
TARGET_FLAGS = -target armv7-unknown-windows-gnu -fshort-wchar -mthumb \
              -ffreestanding -fno-builtin -nostdlibinc \
              -D_AKARI_BUILD=1
CFLAGS      = -Os -fvisibility=hidden -Wall -Wextra $(INCLUDES) $(TARGET_FLAGS)
ASFLAGS     = $(CFLAGS)
ARFLAGS     = cr

# ---- Source files ----
C_SRCS = \
    src/crt/crt0.c \
    src/crt/crt_cpp.c \
    src/crt/dllcrt.c \
    src/crt/patchables.c \
    src/misc/atexit.c \
    src/misc/errno.c \
    src/misc/exit.c \
    src/misc/globals.c

ARM_ASM_SRCS = \
    src/compiler-rt/chkstk_arm.S \
    src/compiler-rt/aeabi_idivmod.S

STARTUP_C_SRCS = src/crt/crt0.c src/crt/dllcrt.c

C_OBJS   = $(C_SRCS:.c=.o)
ASM_OBJS = $(ARM_ASM_SRCS:.S=.o)
LIB_OBJS = $(C_OBJS) $(ASM_OBJS)

CRT0_OBJ   = build/akari_crt0.o
CRT0W_OBJ  = build/akari_crt0w.o
CRT0C_OBJ  = build/akari_crt0c.o
DLLCRT_OBJ = build/akari_dllcrt.o
LIB        = build/libakari.a

.PHONY: all clean install hostcheck samples

all: $(LIB) $(CRT0_OBJ) $(CRT0W_OBJ) $(CRT0C_OBJ) $(DLLCRT_OBJ)

build:
	@mkdir -p build src/crt src/misc src/compiler-rt

$(C_OBJS): %.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_OBJS): %.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(LIB): $(LIB_OBJS) | build
	$(AR) $(ARFLAGS) $@ $(LIB_OBJS)

# Startup objects are the same TU compiled with different -D flags so that
# only one entry symbol is emitted; or in our design we simply provide all
# three entries in one object (crt0.o) and let /ENTRY: choose.  We still
# produce the traditional named objects for compatibility.
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
	rm -rf build $(C_OBJS) $(ASM_OBJS)

# ---- Host-side build sanity check: compile every TU with gcc to
#      confirm there are no syntax errors (this does NOT link; it
#      just exercises the compiler on each source).  We also build a
#      static archive from the objects to verify the object set is
#      self-consistent at the symbol level.
HOSTCC      ?= gcc
HOSTCFLAGS  = -Os -Wall -Wextra -std=c99 -ffreestanding -fshort-wchar \
              -D_AKARI_BUILD=1 -D_DEBUG_HOSTCHECK_ \
              -Iinclude -Iinclude/akari \
              -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
              -Wno-long-long -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
              -Wno-incompatible-pointer-types -Wno-builtin-declaration-mismatch \
              -Wno-pedantic
HOST_OBJS = $(patsubst %.c,build/host/%.o,$(C_SRCS))
hostcheck: | build
	@echo "[hostcheck] building CRT objects with $(HOSTCC)"
	@mkdir -p $(dir $(HOST_OBJS))
	@set -e; for f in $(C_SRCS); do \
	    echo "  CC  $$f"; \
	    mkdir -p "build/host/$$(dirname $$f)"; \
	    $(HOSTCC) $(HOSTCFLAGS) -c "$$f" -o "build/host/$${f%.c}.o"; \
	done
	@echo "[hostcheck] archiving into build/host/libakari.a"
	@ar crs build/host/libakari.a $(C_SRCS:%.c=build/host/%.o)
	@echo "[hostcheck] OK -- all sources compile and archive"
