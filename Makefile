#
#  Stack standalone Makefile
#

# Default target is to build the library
all: libopenblue.a demo $(if $(filter y 1,$(CONFIG_OPENBLUE_BT_CMD)),btcmd)

# Allow overriding toolchain from environment
CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
NM      := $(CROSS_COMPILE)nm
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

# Build flags
# CFLAGS for compiler options, CPPFLAGS for preprocessor options (like -I)
CFLAGS ?= -g -O2 -Wall
CPPFLAGS ?=
# Add flags for dependency generation
CFLAGS += -MMD -MP

# Add flags for GNU source compatibility
CFLAGS += -D_GNU_SOURCE

# Generated config header, must be after .config include
AUTOCONF_H = include/generated/autoconf.h

# Include Kconfig configuration
# This will define CONFIG_... variables
-include .config

# Export them to be available for shell commands in rules
export $(shell sed -e 's/=.*//' -e '/^$$/d' .config)

# Include Directories
CPPFLAGS += -Iinclude
CPPFLAGS += -Iinclude/generated
CPPFLAGS += -Ibluetooth
CPPFLAGS += -Ibluetooth/host
CPPFLAGS += -Ibluetooth/host/classic
CPPFLAGS += -Ibluetooth/audio
CPPFLAGS += -Ibluetooth/mesh
CPPFLAGS += -Ibluetooth/services
CPPFLAGS += -Ibluetooth/lib
CPPFLAGS += -Ibluetooth/services/bas
CPPFLAGS += -Ibluetooth/services/ias
CPPFLAGS += -Ibluetooth/services/nus
CPPFLAGS += -Ibluetooth/services/ots
CPPFLAGS += -Ishim/include
CPPFLAGS += -I.

# Force include the generated config file for all source files
CPPFLAGS += -include $(AUTOCONF_H)

CPPFLAGS += -include shim/include/shim.h

ifneq ($(CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS),)
# Prefer system mbed TLS via pkg-config (mbedcrypto or mbedtls). Fallback: local pinned build.
MBEDTLS_PKG_AVAILABLE := $(shell (pkg-config --exists mbedcrypto || pkg-config --exists mbedtls) && echo yes || echo no)

ifeq ($(MBEDTLS_PKG_AVAILABLE),yes)
  MBEDTLS_CFLAGS := $(shell pkg-config --cflags mbedcrypto 2>/dev/null || pkg-config --cflags mbedtls 2>/dev/null)
  MBEDTLS_LIBS   := $(shell pkg-config --libs mbedcrypto 2>/dev/null || pkg-config --libs mbedtls 2>/dev/null)
else
  MBEDTLS_LOCAL_DIR := third_party/mbedtls
  MBEDTLS_LOCAL_TAG := v2.28.8
  MBEDTLS_LOCAL_CRYPTO := $(MBEDTLS_LOCAL_DIR)/library/libmbedcrypto.a
  MBEDTLS_FALLBACK_TARGET := mbedtls-local
  MBEDTLS_CFLAGS := -I$(MBEDTLS_LOCAL_DIR)/include
  MBEDTLS_LIBS   := $(MBEDTLS_LOCAL_CRYPTO)
endif

# Enable PSA via mbed TLS and propagate CFLAGS/INCLUDEs
CFLAGS += $(MBEDTLS_CFLAGS)
endif

# Fallback: local mbed TLS build target (only defined when used)
ifeq ($(MBEDTLS_FALLBACK_TARGET),mbedtls-local)
.PHONY: mbedtls-local
mbedtls-local:
	@echo "Fetching and building local mbed TLS ($(MBEDTLS_LOCAL_TAG))..."
	@test -d $(MBEDTLS_LOCAL_DIR) || git clone --depth 1 --branch $(MBEDTLS_LOCAL_TAG) https://github.com/Mbed-TLS/mbedtls.git $(MBEDTLS_LOCAL_DIR)
	@$(MAKE) -C $(MBEDTLS_LOCAL_DIR)/library libmbedcrypto.a
endif

# Source files
CSRCS :=

# Core init registry
CSRCS += core/stack_init.c

# Base sources (kept in root lib)
CSRCS_BASE :=
CSRCS_BASE += base/bt_atomic.c
CSRCS_BASE += base/bt_buf.c
CSRCS_BASE += base/bt_buf_simple.c
CSRCS_BASE += base/utils.c
CSRCS_BASE += base/queue/bt_queue.c
CSRCS_BASE += base/bt_mem_pool.c
CSRCS_BASE += base/bt_poll.c
CSRCS_BASE += base/bt_work.c
CSRCS_BASE += base/bt_debug.c
CSRCS_BASE += base/log.c

# Integrate Bluetooth module (platform selection and sources)
BT_PLATFORM ?= linux
include bluetooth/module.mk

# Append module include flags
CPPFLAGS += $(BT_CPPFLAGS)

# Combine sources: base + bluetooth module
CSRCS += \
    $(CSRCS_BASE) \
    $(BT_SRCS)

# Object files
OBJS = $(CSRCS:.c=.o)

# Generated dependency files
DEPS = $(OBJS:.o=.d)

# Ensure local mbed TLS headers/libs are ready before compiling objects
ifeq ($(MBEDTLS_FALLBACK_TARGET),mbedtls-local)
$(OBJS): | mbedtls-local
endif

# Make all objects depend on the generated config header
$(OBJS): $(AUTOCONF_H)

# Rule to generate the config header
$(AUTOCONF_H): .config kconfig-deps
	@echo "Generating config header $< -> $@"
	@mkdir -p $(dir $@)
	@python3 tools/kconfig/genconfig.py --header-output $@ .config

# Main build target
libopenblue.a: $(OBJS)
	@echo "AR $@"
	$(AR) rcs $@ $(OBJS)

# Rule to compile a .c file to a .o file
%.o: %.c
	@echo "CC $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Include generated dependency files
-include $(DEPS)

all: libopenblue.a demo btcmd

# Clean target
clean:
	@echo "Cleaning..."
	$(RM) $(OBJS) $(DEPS) libopenblue.a samples/demo/demo samples/demo/demo.d
	$(RM) samples/cmds/btcmd samples/cmds/btcmd.d
	$(RM) -r include/generated

# Samples targets
.PHONY: samples demo btcmd

samples: demo btcmd

ifeq ($(CONFIG_OPENBLUE_SAMPLES), y)
demo: $(MBEDTLS_FALLBACK_TARGET) libopenblue.a samples/demo/main.c
	@echo "Building samples/demo..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -o samples/demo/demo samples/demo/main.c libopenblue.a $(MBEDTLS_LIBS) -lpthread -lrt
else
	@echo "Skipping samples/demo..."
endif

ifeq ($(CONFIG_OPENBLUE_BT_CMD), y)
btcmd: $(MBEDTLS_FALLBACK_TARGET) libopenblue.a samples/cmds/bt_cmd_main.c
	@echo "Building samples/cmds/btcmd..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -o samples/cmds/btcmd samples/cmds/bt_cmd_main.c libopenblue.a $(MBEDTLS_LIBS) -lpthread -lrt
else
btcmd:
	@echo "Skipping samples/cmds/btcmd..."
endif

# Kconfig targets
.PHONY: menuconfig kconfig-deps help genconfig

help:
	@echo "Targets:"
	@echo "  make              - build libopenblue.a"
	@echo "  make menuconfig   - run Kconfig menuconfig UI and write .config"
	@echo "  make genconfig    - force generate header file from .config"
	@echo "  make clean        - remove built files"
	@echo "  make demo         - build samples/demo"
ifeq ($(filter y 1,$(CONFIG_OPENBLUE_BT_CMD)),y 1)
	@echo "  make btcmd        - build samples/cmds/btcmd"
endif
	@echo "  make all          - build all"

kconfig-deps:
	@python3 -c "import kconfiglib" 2>/dev/null || pip3 install --user kconfiglib

menuconfig: kconfig-deps
	@python3 tools/kconfig/run_menuconfig.py Kconfig .config