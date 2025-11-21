#ifndef __BASE_QUEUE_H__
#define __BASE_QUEUE_H__

#include <stdbool.h>
#include <stddef.h>

#include "osdep/os.h"
#include "utils/bt_slist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque queue type for consumers */
struct bt_queue {
	bt_slist_t list; /* singly linked list of queue_node */
	os_mutex_t lock;  /* protects list & cond */
	os_cond_t cond;   /* signaled when data becomes available */
	bt_dlist_t poll_events;
};

#define BT_QUEUE_INITIALIZER(obj)                                                                  \
	{                                                                                          \
		.list = BT_SLIST_STATIC_INIT(&obj.list), .lock = OS_MUTEX_INITIALIZER,            \
		.cond = OS_COND_INITIALIZER,                                                       \
		.poll_events = BT_DLIST_STATIC_INIT(&obj.poll_events),                            \
	}

/* Initialize a queue object */
void bt_queue_init(struct bt_queue *queue);

/* Append item to tail (FIFO) */
void bt_queue_append(struct bt_queue *queue, void *data);

void bt_queue_prepend(struct bt_queue *queue, void *data);

void bt_queue_cancel_wait(struct bt_queue *queue);

/* Remove and return head item; blocks up to timeout if empty. */
void *bt_queue_get(struct bt_queue *queue, os_timeout_t timeout);

/* Inspect without removal */
void *bt_queue_peek_head(struct bt_queue *queue);
void *bt_queue_peek_tail(struct bt_queue *queue);

/* Remove a specific data pointer if present; returns true if removed. */
bool bt_queue_remove(struct bt_queue *queue, void *data);

/* Append only if data not already present; returns true if appended, false if
 * duplicate. */
bool bt_queue_unique_append(struct bt_queue *queue, void *data);

/* Is the queue empty? */
bool bt_queue_is_empty(struct bt_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* __BASE_QUEUE_H__ */
