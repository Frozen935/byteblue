/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>

#include <bluetooth/mesh.h>

#include "msg.h"
#include "access.h"
#include "net.h"

#define LOG_LEVEL CONFIG_BT_MESH_ACCESS_LOG_LEVEL

static void delayable_msg_handler(struct bt_work *w);
static bool push_msg_from_delayable_msgs(void);

static struct delayable_msg_chunk {
	bt_snode_t node;
	uint8_t data[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE];
} delayable_msg_chunks[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT];

static struct delayable_msg_ctx {
	bt_snode_t node;
	bt_slist_t chunks;
	struct bt_mesh_msg_ctx ctx;
	uint16_t src_addr;
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
	uint32_t fired_time;
	uint16_t len;
} delayable_msgs_ctx[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT];

static struct {
	bt_slist_t busy_ctx;
	bt_slist_t free_ctx;
	bt_slist_t free_chunks;
	struct bt_work_delayable random_delay;
} access_delayable_msg = {.random_delay = BT_WORK_DELAYABLE_INITIALIZER(delayable_msg_handler)};

static void put_ctx_to_busy_list(struct delayable_msg_ctx *ctx)
{
	struct delayable_msg_ctx *curr_ctx;
	bt_slist_t *list = &access_delayable_msg.busy_ctx;
	bt_snode_t *head = bt_slist_peek_head(list);
	bt_snode_t *curr = head;
	bt_snode_t *prev = curr;

	if (!head) {
		bt_slist_append(list, &ctx->node);
		return;
	}

	do {
		curr_ctx = CONTAINER_OF(curr, struct delayable_msg_ctx, node);
		if (ctx->fired_time < curr_ctx->fired_time) {
			if (curr == head) {
				bt_slist_prepend(list, &ctx->node);
			} else {
				bt_slist_insert(list, prev, &ctx->node);
			}
			return;
		}
		prev = curr;
	} while ((curr = bt_slist_peek_next(curr)));

	bt_slist_append(list, &ctx->node);
}

static struct delayable_msg_ctx *peek_pending_msg(void)
{
	struct delayable_msg_ctx *pending_msg = NULL;
	bt_snode_t *node = bt_slist_peek_head(&access_delayable_msg.busy_ctx);

	if (node) {
		pending_msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	}

	return pending_msg;
}

static void reschedule_delayable_msg(struct delayable_msg_ctx *msg)
{
	uint32_t curr_time;
	os_timeout_t delay = OS_TIMEOUT_NO_WAIT;
	struct delayable_msg_ctx *pending_msg;

	if (msg) {
		put_ctx_to_busy_list(msg);
	}

	pending_msg = peek_pending_msg();

	if (!pending_msg) {
		return;
	}

	curr_time = k_uptime_get_32();
	if (curr_time < pending_msg->fired_time) {
		delay = OS_MSEC(pending_msg->fired_time - curr_time);
	}

	bt_work_reschedule(&access_delayable_msg.random_delay, delay);
}

static int allocate_delayable_msg_chunks(struct delayable_msg_ctx *msg, int number)
{
	bt_snode_t *node;

	for (int i = 0; i < number; i++) {
		node = bt_slist_get(&access_delayable_msg.free_chunks);
		if (!node) {
			LOG_WRN("Unable allocate %u chunks, allocated %u", number, i);
			return i;
		}
		bt_slist_append(&msg->chunks, node);
	}

	return number;
}

static void release_delayable_msg_chunks(struct delayable_msg_ctx *msg)
{
	bt_snode_t *node;

	while ((node = bt_slist_get(&msg->chunks))) {
		bt_slist_append(&access_delayable_msg.free_chunks, node);
	}
}

static struct delayable_msg_ctx *allocate_delayable_msg_ctx(void)
{
	struct delayable_msg_ctx *msg;
	bt_snode_t *node;

	if (bt_slist_is_empty(&access_delayable_msg.free_ctx)) {
		LOG_WRN("Purge pending delayable message.");
		if (!push_msg_from_delayable_msgs()) {
			return NULL;
		}
	}

	node = bt_slist_get(&access_delayable_msg.free_ctx);
	msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	bt_slist_init(&msg->chunks);

	return msg;
}

static void release_delayable_msg_ctx(struct delayable_msg_ctx *ctx)
{
	if (bt_slist_find_and_remove(&access_delayable_msg.busy_ctx, &ctx->node)) {
		bt_slist_append(&access_delayable_msg.free_ctx, &ctx->node);
	}
}

static bool push_msg_from_delayable_msgs(void)
{
	bt_snode_t *node;
	struct delayable_msg_chunk *chunk;
	struct delayable_msg_ctx *msg = peek_pending_msg();
	uint16_t len;
	int err;

	if (!msg) {
		return false;
	}

	len = msg->len;

	BT_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);

	BT_SLIST_FOR_EACH_NODE(&msg->chunks, node) {
		uint16_t tmp = MIN((uint16_t)CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE, len);

		chunk = CONTAINER_OF(node, struct delayable_msg_chunk, node);
		memcpy(bt_buf_simple_add(&buf, tmp), chunk->data, tmp);
		len -= tmp;
	}

	msg->ctx.rnd_delay = false;
	err = bt_mesh_access_send(&msg->ctx, &buf, msg->src_addr, msg->cb, msg->cb_data);
	msg->ctx.rnd_delay = true;

	if (err == -EBUSY || err == -ENOBUFS) {
		return false;
	}

	release_delayable_msg_chunks(msg);
	release_delayable_msg_ctx(msg);

	if (err && msg->cb && msg->cb->start) {
		msg->cb->start(0, err, msg->cb_data);
	}

	return true;
}

static void delayable_msg_handler(struct bt_work *w)
{
	if (!push_msg_from_delayable_msgs()) {
		bt_snode_t *node = bt_slist_get(&access_delayable_msg.busy_ctx);
		struct delayable_msg_ctx *pending_msg =
			CONTAINER_OF(node, struct delayable_msg_ctx, node);

		pending_msg->fired_time += 10;
		reschedule_delayable_msg(pending_msg);
	} else {
		reschedule_delayable_msg(NULL);
	}
}

int bt_mesh_delayable_msg_manage(struct bt_mesh_msg_ctx *ctx, struct bt_buf_simple *buf,
				 uint16_t src_addr, const struct bt_mesh_send_cb *cb, void *cb_data)
{
	bt_snode_t *node;
	struct delayable_msg_ctx *msg;
	uint16_t random_delay;
	int total_number = DIV_ROUND_UP(buf->size, CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE);
	int allocated_number = 0;
	uint16_t len = buf->len;

	if (bt_atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		LOG_WRN("Refusing to allocate message context while suspended");
		return -ENODEV;
	}

	if (total_number > CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT) {
		return -EINVAL;
	}

	msg = allocate_delayable_msg_ctx();
	if (!msg) {
		LOG_WRN("No available free delayable message context.");
		return -ENOMEM;
	}

	do {
		allocated_number +=
			allocate_delayable_msg_chunks(msg, total_number - allocated_number);

		if (total_number > allocated_number) {
			LOG_DBG("Unable allocate %u chunks, allocated %u", total_number,
				allocated_number);
			if (!push_msg_from_delayable_msgs()) {
				LOG_WRN("No available chunk memory.");
				release_delayable_msg_chunks(msg);
				release_delayable_msg_ctx(msg);
				return -ENOMEM;
			}
		}
	} while (total_number > allocated_number);

	BT_SLIST_FOR_EACH_NODE(&msg->chunks, node) {
		uint16_t tmp = MIN(CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE, buf->len);

		struct delayable_msg_chunk *chunk =
			CONTAINER_OF(node, struct delayable_msg_chunk, node);

		memcpy(chunk->data, bt_buf_simple_pull_mem(buf, tmp), tmp);
	}

	bt_rand(&random_delay, sizeof(uint16_t));
	random_delay = 20 + random_delay % (BT_MESH_ADDR_IS_UNICAST(ctx->recv_dst) ? 30 : 480);
	msg->fired_time = k_uptime_get_32() + random_delay;
	msg->ctx = *ctx;
	msg->src_addr = src_addr;
	msg->cb = cb;
	msg->cb_data = cb_data;
	msg->len = len;

	reschedule_delayable_msg(msg);

	return 0;
}

void bt_mesh_delayable_msg_init(void)
{
	bt_slist_init(&access_delayable_msg.busy_ctx);
	bt_slist_init(&access_delayable_msg.free_ctx);
	bt_slist_init(&access_delayable_msg.free_chunks);

	for (int i = 0; i < CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT; i++) {
		bt_slist_append(&access_delayable_msg.free_ctx, &delayable_msgs_ctx[i].node);
	}

	for (int i = 0; i < CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT; i++) {
		bt_slist_append(&access_delayable_msg.free_chunks, &delayable_msg_chunks[i].node);
	}
}

void bt_mesh_delayable_msg_stop(void)
{
	bt_snode_t *node;
	struct delayable_msg_ctx *ctx;

	bt_work_cancel_delayable(&access_delayable_msg.random_delay);

	while ((node = bt_slist_peek_head(&access_delayable_msg.busy_ctx))) {
		ctx = CONTAINER_OF(node, struct delayable_msg_ctx, node);
		release_delayable_msg_chunks(ctx);
		release_delayable_msg_ctx(ctx);

		if (ctx->cb && ctx->cb->start) {
			ctx->cb->start(0, -ENODEV, ctx->cb_data);
		}
	}
}
