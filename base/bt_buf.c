#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <base/byteorder.h>
#include <base/bt_buf.h>

#if defined(CONFIG_BT_BUF_LOG)
#define BT_BUF_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#define BT_BUF_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#else
#define BT_BUF_DBG(fmt, ...)
#define BT_BUF_ERR(fmt, ...)
#endif
#define GET_ALIGN(pool) MAX(sizeof(void *), pool->alloc->alignment)

int bt_buf_id(const struct bt_buf *buf)
{
	struct bt_buf_pool *pool = buf->pool;
	size_t struct_size =
		ROUND_UP(sizeof(struct bt_buf) + pool->user_data_size, __alignof__(struct bt_buf));
	ptrdiff_t offset = (uint8_t *)buf - (uint8_t *)pool->__bufs;

	return offset / struct_size;
}

static inline struct bt_buf *pool_get_uninit(struct bt_buf_pool *pool, uint16_t uninit_count)
{
	size_t struct_size =
		ROUND_UP(sizeof(struct bt_buf) + pool->user_data_size, __alignof__(struct bt_buf));
	size_t byte_offset = (pool->buf_count - uninit_count) * struct_size;
	struct bt_buf *buf;

	buf = (struct bt_buf *)(((uint8_t *)pool->__bufs) + byte_offset);

	buf->pool = pool;
	buf->user_data_size = pool->user_data_size;

	return buf;
}

void bt_buf_reset(struct bt_buf *buf)
{
	assert(buf->flags == 0U);
	assert(buf->frags == NULL);

	bt_buf_simple_reset(&buf->b);
}

#if 0
static uint8_t *generic_data_ref(struct bt_buf *buf, uint8_t *data)
{
	struct bt_buf_pool *buf_pool = buf->pool;
	uint8_t *ref_count;

	ref_count = data - GET_ALIGN(buf_pool);
	(*ref_count)++;

	return data;
}

static uint8_t *mem_pool_data_alloc(struct bt_buf *buf, size_t *size, int32_t timeout)
{
	struct bt_buf_pool *buf_pool = buf->pool;
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;
	void *b;

	if (buf_pool->alloc->alignment == 0) {
		/* Reserve extra space for a ref-count (uint8_t) */
		b = k_heap_alloc(pool, sizeof(void *) + *size, timeout);

	} else {
		if (*size < buf_pool->alloc->alignment) {
			return NULL;
		}

		/* Reserve extra space for a ref-count (uint8_t) */
		b = k_heap_aligned_alloc(
			pool, buf_pool->alloc->alignment,
			GET_ALIGN(buf_pool) + ROUND_UP(*size, buf_pool->alloc->alignment), timeout);
	}

	if (b == NULL) {
		return NULL;
	}

	ref_count = (uint8_t *)b;
	*ref_count = 1U;

	/* Return pointer to the byte following the ref count */
	return ref_count + GET_ALIGN(buf_pool);
}

static void mem_pool_data_unref(struct bt_buf *buf, uint8_t *data)
{
	struct bt_buf_pool *buf_pool = buf->pool;
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;

	ref_count = data - GET_ALIGN(buf_pool);
	if (--(*ref_count)) {
		return;
	}

	/* Need to copy to local variable due to alignment */
	k_heap_free(pool, ref_count);
}

const struct bt_buf_data_cb bt_buf_var_cb = {
	.alloc = mem_pool_data_alloc,
	.ref = generic_data_ref,
	.unref = mem_pool_data_unref,
};
#endif

static uint8_t *fixed_data_alloc(struct bt_buf *buf, size_t *size, int32_t timeout)
{
	struct bt_buf_pool *pool = buf->pool;
	const struct bt_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	*size = pool->alloc->max_alloc_size;

	return fixed->data_pool + *size * bt_buf_id(buf);
}

static void fixed_data_unref(struct bt_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
}

const struct bt_buf_data_cb bt_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

#if (K_HEAP_MEM_POOL_SIZE > 0)

static uint8_t *heap_data_alloc(struct bt_buf *buf, size_t *size, int32_t timeout)
{
	struct bt_buf_pool *buf_pool = buf->pool;
	uint8_t *ref_count;

	ref_count = os_malloc(GET_ALIGN(buf_pool) + *size);
	if (!ref_count) {
		return NULL;
	}

	*ref_count = 1U;

	return ref_count + GET_ALIGN(buf_pool);
}

static void heap_data_unref(struct bt_buf *buf, uint8_t *data)
{
	struct bt_buf_pool *buf_pool = buf->pool;
	uint8_t *ref_count;

	ref_count = data - GET_ALIGN(buf_pool);
	if (--(*ref_count)) {
		return;
	}

	os_free(ref_count);
}

static const struct bt_buf_data_cb bt_buf_heap_cb = {
	.alloc = heap_data_alloc,
	.ref = generic_data_ref,
	.unref = heap_data_unref,
};

const struct bt_buf_data_alloc bt_buf_heap_alloc = {
	.cb = &bt_buf_heap_cb,
	.max_alloc_size = 0,
};

#endif /* K_HEAP_MEM_POOL_SIZE > 0 */

static uint8_t *data_alloc(struct bt_buf *buf, size_t *size, int32_t timeout)
{
	struct bt_buf_pool *pool = buf->pool;

	return pool->alloc->cb->alloc(buf, size, timeout);
}

static uint8_t *data_ref(struct bt_buf *buf, uint8_t *data)
{
	struct bt_buf_pool *pool = buf->pool;

	return pool->alloc->cb->ref(buf, data);
}

#if defined(CONFIG_BT_BUF_LOG)
struct bt_buf *bt_buf_alloc_len_debug(struct bt_buf_pool *pool, size_t size, int32_t timeout,
				      const char *func, int line)

#else
struct bt_buf *bt_buf_alloc_len(struct bt_buf_pool *pool, size_t size, int32_t timeout)
#endif
{
	struct bt_buf *buf;

	assert(pool);

	BT_BUF_DBG("%s():%d: pool %p size %zu", func, line, pool, size);

	/* We need to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	os_mutex_lock(&pool->lock, OS_TIMEOUT_FOREVER);

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	if (pool->uninit_count) {
		uint16_t uninit_count;

		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the queue with OS_TIMEOUT_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = bt_queue_get(&pool->free, OS_TIMEOUT_NO_WAIT);
			if (buf) {
				os_mutex_unlock(&pool->lock);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		os_mutex_unlock(&pool->lock);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	os_mutex_unlock(&pool->lock);

	buf = bt_queue_get(&pool->free, timeout);
	if (!buf) {
		BT_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	BT_BUF_DBG("allocated buf %p", buf);

	if (size) {
		__maybe_unused size_t req_size = size;

		timeout = (timeout != OS_TIMEOUT_NO_WAIT) ? OS_TIMEOUT_FOREVER : OS_TIMEOUT_NO_WAIT;
		buf->__buf = data_alloc(buf, &size, timeout);
		if (!buf->__buf) {
			BT_BUF_ERR("%s():%d: Failed to allocate data", func, line);
			bt_buf_destroy(buf);
			return NULL;
		}

		assert(req_size <= size);
	} else {
		buf->__buf = NULL;
	}

	buf->ref = 1U;
	buf->flags = 0U;
	buf->frags = NULL;
	buf->size = size;
	memset(buf->user_data, 0, buf->user_data_size);
	bt_buf_reset(buf);

	return buf;
}

#if defined(CONFIG_BT_BUF_LOG)
struct bt_buf *bt_buf_alloc_fixed_debug(struct bt_buf_pool *pool, int32_t timeout, const char *func,
					int line)
{
	return bt_buf_alloc_len_debug(pool, pool->alloc->max_alloc_size, timeout, func, line);
}
#else
struct bt_buf *bt_buf_alloc_fixed(struct bt_buf_pool *pool, int32_t timeout)
{
	return bt_buf_alloc_len(pool, pool->alloc->max_alloc_size, timeout);
}
#endif

struct bt_buf *bt_buf_alloc_with_data(struct bt_buf_pool *pool, void *data, size_t size,
				      int32_t timeout)
{
	struct bt_buf *buf;

	buf = bt_buf_alloc_len(pool, 0, timeout);
	if (!buf) {
		return NULL;
	}

	bt_buf_simple_init_with_data(&buf->b, data, size);
	buf->flags = BT_BUF_EXTERNAL_DATA;

	return buf;
}

static os_mutex_t bt_buf_slist_lock = OS_MUTEX_INITIALIZER;

void bt_buf_slist_put(bt_slist_t *list, struct bt_buf *buf)
{
	assert(list);
	assert(buf);

	os_mutex_lock(&bt_buf_slist_lock, OS_TIMEOUT_FOREVER);
	bt_slist_append(list, &buf->node);
	os_mutex_unlock(&bt_buf_slist_lock);
}

struct bt_buf *bt_buf_slist_get(bt_slist_t *list)
{
	struct bt_buf *buf;

	assert(list);

	os_mutex_lock(&bt_buf_slist_lock, OS_TIMEOUT_FOREVER);

	buf = (void *)bt_slist_get(list);

	os_mutex_unlock(&bt_buf_slist_lock);

	return buf;
}

#if defined(CONFIG_BT_BUF_LOG)
void bt_buf_unref_debug(struct bt_buf *buf, const char *func, int line)
#else
void bt_buf_unref(struct bt_buf *buf)
#endif
{
	assert(buf);

	while (buf) {
		struct bt_buf *frags = buf->frags;
		struct bt_buf_pool *pool;

		assert(buf->ref);
		if (!buf->ref) {
			BT_BUF_ERR("%s():%d: buf %p double free", func, line, buf);
			return;
		}

		BT_BUF_DBG("%s():%d: buf %p ref %u pool %p frags %p", func, line, buf, buf->ref,
			   buf->pool, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		buf->data = NULL;
		buf->frags = NULL;

		pool = buf->pool;

		if (pool->destroy) {
			pool->destroy(buf);
		} else {
			bt_buf_destroy(buf);
		}

		buf = frags;
	}
}

struct bt_buf *bt_buf_ref(struct bt_buf *buf)
{
	assert(buf);

	BT_BUF_DBG("buf %p (old) ref %u pool %p", buf, buf->ref, buf->pool);
	buf->ref++;
	return buf;
}

struct bt_buf *bt_buf_clone(struct bt_buf *buf, int32_t timeout)
{
	struct bt_buf_pool *pool;
	struct bt_buf *clone;

	assert(buf);

	pool = buf->pool;

	clone = bt_buf_alloc_len(pool, 0, timeout);
	if (!clone) {
		return NULL;
	}

	/* If the pool supports data referencing use that. Otherwise
	 * we need to allocate new data and make a copy.
	 */
	if (pool->alloc->cb->ref && !(buf->flags & BT_BUF_EXTERNAL_DATA)) {
		clone->__buf = buf->__buf ? data_ref(buf, buf->__buf) : NULL;
		clone->data = buf->data;
		clone->len = buf->len;
		clone->size = buf->size;
	} else {
		size_t size = buf->size;

		timeout = (timeout != OS_TIMEOUT_NO_WAIT) ? OS_TIMEOUT_FOREVER : OS_TIMEOUT_NO_WAIT;

		clone->__buf = data_alloc(clone, &size, timeout);
		if (!clone->__buf || size < buf->size) {
			bt_buf_destroy(clone);
			return NULL;
		}

		clone->size = size;
		clone->data = clone->__buf + bt_buf_headroom(buf);
		bt_buf_add_mem(clone, buf->data, buf->len);
	}

	/* user_data_size should be the same for buffers from the same pool */
	assert(buf->user_data_size == clone->user_data_size);

	memcpy(clone->user_data, buf->user_data, clone->user_data_size);

	return clone;
}

int bt_buf_user_data_copy(struct bt_buf *dst, const struct bt_buf *src)
{
	assert(dst);
	assert(src);

	if (dst == src) {
		return 0;
	}

	if (dst->user_data_size < src->user_data_size) {
		return -EINVAL;
	}

	memcpy(dst->user_data, src->user_data, src->user_data_size);

	return 0;
}

struct bt_buf *bt_buf_frag_last(struct bt_buf *buf)
{
	assert(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void bt_buf_frag_insert(struct bt_buf *parent, struct bt_buf *frag)
{
	assert(parent);
	assert(frag);

	if (parent->frags) {
		bt_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct bt_buf *bt_buf_frag_add(struct bt_buf *head, struct bt_buf *frag)
{
	assert(frag);

	if (!head) {
		return bt_buf_ref(frag);
	}

	bt_buf_frag_insert(bt_buf_frag_last(head), frag);

	return head;
}

struct bt_buf *bt_buf_frag_del(struct bt_buf *parent, struct bt_buf *frag)
{
	struct bt_buf *next_frag;

	assert(frag);

	if (parent) {
		assert(parent->frags);
		assert(parent->frags == frag);
		parent->frags = frag->frags;
	}

	next_frag = frag->frags;

	frag->frags = NULL;
	bt_buf_unref(frag);

	return next_frag;
}

size_t bt_buf_linearize(void *dst, size_t dst_len, const struct bt_buf *src, size_t offset,
			size_t len)
{
	const struct bt_buf *frag;
	size_t to_copy;
	size_t copied;

	len = MIN(len, dst_len);

	frag = src;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		memcpy((uint8_t *)dst + copied, frag->data + offset, to_copy);

		copied += to_copy;

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	return copied;
}

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
size_t bt_buf_append_bytes(struct bt_buf *buf, size_t len, const void *value, int32_t timeout,
			   bt_buf_allocator_cb allocate_cb, void *user_data)
{
	struct bt_buf *frag = bt_buf_frag_last(buf);
	size_t added_len = 0;
	const uint8_t *value8 = value;
	size_t max_size;

	do {
		uint16_t count = MIN(len, bt_buf_tailroom(frag));

		bt_buf_add_mem(frag, value8, count);
		len -= count;
		added_len += count;
		value8 += count;

		if (len == 0) {
			return added_len;
		}

		if (allocate_cb) {
			frag = allocate_cb(timeout, user_data);
		} else {
			struct bt_buf_pool *pool;

			/* Allocate from the original pool if no callback has
			 * been provided.
			 */
			pool = buf->pool;
			max_size = pool->alloc->max_alloc_size;
			frag = bt_buf_alloc_len(pool, max_size ? MIN(len, max_size) : len, timeout);
		}

		if (!frag) {
			return added_len;
		}

		bt_buf_frag_add(buf, frag);
	} while (1);

	/* Unreachable */
	return 0;
}

size_t bt_buf_data_match(const struct bt_buf *buf, size_t offset, const void *data, size_t len)
{
	const uint8_t *dptr = data;
	const uint8_t *bptr;
	size_t compared = 0;
	size_t to_compare;

	if (!buf || !data) {
		return compared;
	}

	/* find the right fragment to start comparison */
	while (buf && offset >= buf->len) {
		offset -= buf->len;
		buf = buf->frags;
	}

	while (buf && len > 0) {
		bptr = buf->data + offset;
		to_compare = MIN(len, buf->len - offset);

		for (size_t i = 0; i < to_compare; ++i) {
			if (dptr[compared] != bptr[i]) {
				return compared;
			}
			compared++;
		}

		len -= to_compare;
		buf = buf->frags;
		offset = 0;
	}

	return compared;
}
