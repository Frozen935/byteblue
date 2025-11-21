#include <stdlib.h>
#include <errno.h>

#include "base/queue/bt_queue.h"

void bt_queue_init(struct bt_queue *queue)
{
	if (!queue) {
		return;
	}
	bt_slist_init(&queue->list);
	/* Initialize lock and cond */
	(void)os_mutex_init(&queue->lock);
	(void)os_cond_init(&queue->cond);
}

void bt_queue_append(struct bt_queue *queue, void *data)
{
	if (!queue) {
		return;
	}

	bt_snode_t *n = (bt_snode_t *)data;
	if (!n) {
		/* Drop silently on OOM to avoid crashing; caller can choose to
		 * check with unique_append */
		return;
	}

	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	bt_slist_append(&queue->list, n);
	/* Wake one waiter */
	os_cond_signal(&queue->cond);
	os_mutex_unlock(&queue->lock);
}

void bt_queue_cancel_wait(struct bt_queue *queue)
{
	if (!queue) {
		return;
	}
	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	os_cond_signal(&queue->cond);
	os_mutex_unlock(&queue->lock);
}

void bt_queue_prepend(struct bt_queue *queue, void *data)
{
	if (!queue) {
		return;
	}

	bt_snode_t *n = (bt_snode_t *)data;
	if (!n) {
		return;
	}
	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	bt_slist_prepend(&queue->list, n);
	/* Wake one waiter */
	os_cond_signal(&queue->cond);
	os_mutex_unlock(&queue->lock);
}

static void *pop_head_unlocked(struct bt_queue *queue)
{
	bt_snode_t *node = bt_slist_get(&queue->list);
	if (!node) {
		return NULL;
	}

	return (void *)node;
}

void *bt_queue_get(struct bt_queue *queue, os_timeout_t timeout)
{
	if (!queue) {
		return NULL;
	}

	int32_t wait_ms = (int32_t)timeout;
	void *ret = NULL;

	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);

	/* Fast path: non-empty */
	ret = pop_head_unlocked(queue);
	if (ret) {
		os_mutex_unlock(&queue->lock);
		return ret;
	}

	/* Empty: handle timeout semantics */
	if (TIMEOUT_EQ(timeout, OS_TIMEOUT_NO_WAIT)) {
		os_mutex_unlock(&queue->lock);
		return NULL;
	}

	if (TIMEOUT_EQ(timeout, OS_TIMEOUT_FOREVER)) {
		/* Block until something arrives */
		while (bt_slist_is_empty(&queue->list)) {
			int rc = os_cond_wait(&queue->cond, &queue->lock, OS_TIMEOUT_FOREVER);
			if (rc != 0 && rc != -EINTR) {
				/* Unexpected error: break and return NULL */
				break;
			}
		}
	} else {
		/* Timed wait in ms */
		int32_t remaining = wait_ms;
		while (bt_slist_is_empty(&queue->list)) {
			int rc = os_cond_wait(&queue->cond, &queue->lock, remaining);
			if (rc == -ETIMEDOUT) {
				break;
			}
			if (rc != 0 && rc != -EINTR) {
				break;
			}
			/* On EINTR, loop and wait for the remaining time;
			 * os_cond_wait already accounts for absolute time */
			/* For simplicity, treat EINTR as spurious and continue
			 */
		}
	}

	ret = pop_head_unlocked(queue);

	os_mutex_unlock(&queue->lock);
	return ret;
}

void *bt_queue_peek_head(struct bt_queue *queue)
{
	if (!queue) {
		return NULL;
	}
	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	bt_snode_t *node = bt_slist_peek_head(&queue->list);
	void *ret = NULL;
	if (node) {
		ret = (void *)node;
	}
	os_mutex_unlock(&queue->lock);
	return ret;
}

void *bt_queue_peek_tail(struct bt_queue *queue)
{
	if (!queue) {
		return NULL;
	}
	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	bt_snode_t *node = bt_slist_peek_tail(&queue->list);
	void *ret = NULL;
	if (node) {
		ret = (void *)node;
	}
	os_mutex_unlock(&queue->lock);
	return ret;
}

bool bt_queue_remove(struct bt_queue *queue, void *data)
{
	if (!queue || !data) {
		return false;
	}
	bt_snode_t *next = NULL;
	bt_snode_t *cur = queue->list.head;
	bt_snode_t *prev = NULL;

	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);

	BT_SLIST_FOR_EACH_NODE_SAFE(&queue->list, cur, next) {
		if (cur == (bt_snode_t *)data) {
			bt_slist_remove(&queue->list, prev, cur);
			os_mutex_unlock(&queue->lock);
			return true;
		}
		prev = cur;
	}

	os_mutex_unlock(&queue->lock);
	return false;
}

bool bt_queue_unique_append(struct bt_queue *queue, void *data)
{
	if (!queue) {
		return false;
	}

	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);

	/* Check for existence */
	bt_snode_t *cur = queue->list.head;
	while (cur) {
		if (cur == (bt_snode_t *)data) {
			os_mutex_unlock(&queue->lock);
			return false;
		}
		cur = cur->next;
	}

	/* Not found: append */
	bt_snode_t *n = (bt_snode_t *)data;
	if (!n) {
		os_mutex_unlock(&queue->lock);
		return false;
	}
	bt_slist_append(&queue->list, n);
	os_cond_signal(&queue->cond);

	os_mutex_unlock(&queue->lock);
	return true;
}

bool bt_queue_is_empty(struct bt_queue *queue)
{
	if (!queue) {
		return true;
	}
	os_mutex_lock(&queue->lock, OS_TIMEOUT_FOREVER);
	bool empty = bt_slist_is_empty(&queue->list);
	os_mutex_unlock(&queue->lock);
	return empty;
}
