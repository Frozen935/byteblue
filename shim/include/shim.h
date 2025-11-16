/*
 * Zephyr compatibility shim (empty/safe definitions)
 * TODO: migrate to Zephyr
 */
#ifndef ZEPHYR_SHIM_SHIM_H
#define ZEPHYR_SHIM_SHIM_H

/* Mark shim presence to avoid duplicate typedefs/macros in local dep headers */
#define ZEPHYR_SHIM_PRESENT 1

/* Standard headers */
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include "bt_toolchain_macro.h"
#include "bt_storage.h"

#if defined(__has_include)
#if __has_include(<bluetooth/addr.h>)
#include <bluetooth/addr.h>
#endif
#endif

/* Bring common local headers into visibility for all units */
#include <utils/bt_slist.h>
#include <utils/bt_dlist.h>

#include <base/bt_debug.h>
#include <base/log.h>
#include <base/byteorder.h>
#include <base/utils.h>
#include <base/assert.h>
#include <base/queue/bt_queue.h>
#include <base/queue/bt_fifo.h>
#include <base/queue/bt_lifo.h>
#include <base/bt_mem_pool.h>
#include <base/bt_poll.h>
#include <base/bt_work.h>
#include <base/bt_atomic.h>
#include <base/bt_buf.h>

#include <bt_stack_init.h>

extern struct bt_work_q main_work_q;

#if defined(__has_include)
#if __has_include(<bluetooth/att.h>)
#include <bluetooth/att.h>
#endif
#if __has_include(<bluetooth/conn.h>)
#include <bluetooth/conn.h>
#endif
#if __has_include(<bluetooth/gatt.h>)
#include <bluetooth/gatt.h>
#endif
#endif
#if defined(__has_include)
#if __has_include(<bluetooth/addr.h>)
#include <bluetooth/addr.h>
#endif
#endif

#endif /* ZEPHYR_SHIM_SHIM_H */