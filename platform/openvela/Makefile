#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

OPENBLUE_ROOT = openblue
# Core init registry
CSRCS += $(OPENBLUE_ROOT)/core/stack_init.c

# Base sources (kept in root lib)
CSRCS_BASE :=
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_atomic.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_buf.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_buf_simple.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/utils.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/queue/bt_queue.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_mem_pool.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_poll.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_work.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/bt_debug.c
CSRCS_BASE += $(OPENBLUE_ROOT)/base/log.c

# Integrate Bluetooth module (platform selection and sources)
BT_PLATFORM ?= nuttx
include $(OPENBLUE_ROOT)/bluetooth/module.mk

# Combine sources: base + bluetooth module
CSRCS += \
    $(CSRCS_BASE) \
    $(BT_SRCS)

# Include Directories
CFLAGS += -I$(OPENBLUE_ROOT)/include
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/host
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/host/classic
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/audio
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/mesh
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/services
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/lib
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/services/bas
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/services/ias
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/services/nus
CFLAGS += -I$(OPENBLUE_ROOT)/bluetooth/services/ots
CFLAGS += -I$(OPENBLUE_ROOT)/shim/include
CFLAGS += -I$(OPENBLUE_ROOT)/.

CFLAGS += -include $(OPENBLUE_ROOT)/shim/include/shim.h

ifeq ($(CONFIG_BT_SHELL),y)
  PROGNAME += btcmd
  MAINSRC += $(OPENBLUE_ROOT)/samples/cmds/bt_cmd_main.c
  PRIORITY  = SCHED_PRIORITY_DEFAULT
  STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
  MODULE    = $(CONFIG_OPENBLUE_BT_CMD)
endif

include $(APPDIR)/Application.mk