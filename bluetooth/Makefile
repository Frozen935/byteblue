# Standalone Bluetooth build Makefile for openblue/bluetooth
# This Makefile allows building libopenblue_bt.a independently.
# It assumes .config exists in OPENBLUE_ROOT and autoconf.h has been generated.

# Default toolchain (override via environment)
CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
NM      := $(CROSS_COMPILE)nm
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

# Common flags
CFLAGS   ?= -g -O2 -Wall
CPPFLAGS ?=
CFLAGS   += -MMD -MP -D_GNU_SOURCE

# 计算仓库根路径，供 .config 与 autoconf 头文件引用
BT_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BT_ROOT ?= $(abspath $(BT_MK_DIR)/..)

-include $(BT_ROOT)/.config
export $(shell sed -e 's/=.*//' -e '/^$$/d' $(BT_ROOT)/.config)

# Include the reusable module
include module.mk

# mbed TLS header support when crypto uses PSA/mbedTLS
ifneq ($(CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS),)
MBEDTLS_PKG_AVAILABLE := $(shell (pkg-config --exists mbedcrypto || pkg-config --exists mbedtls) && echo yes || echo no)
ifeq ($(MBEDTLS_PKG_AVAILABLE),yes)
  MBEDTLS_CFLAGS := $(shell pkg-config --cflags mbedcrypto 2>/dev/null || pkg-config --cflags mbedtls 2>/dev/null)
else
  MBEDTLS_LOCAL_DIR := $(OPENBLUE_ROOT)/third_party/mbedtls
  MBEDTLS_LOCAL_TAG := v2.28.8
  MBEDTLS_CFLAGS    := -I$(MBEDTLS_LOCAL_DIR)/include
endif
CFLAGS += $(MBEDTLS_CFLAGS)
endif

# Propagate module preprocessor flags
CPPFLAGS += $(BT_CPPFLAGS)

# Pattern rule to compile C -> O
%.o: %.c
	@echo "CC $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Include generated dependency files
-include $(BT_DEPS)

# Default target
.PHONY: all clean
all: libopenblue_bt.a

clean: bt-clean
