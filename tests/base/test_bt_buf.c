#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>

#if defined(__has_include)
#if __has_include(<cmocka.h>)
#include <cmocka.h>
#else
#include <cmocka.h>
#endif
#else
#include <cmocka.h>
#endif

#include <base/bt_buf.h>
#include <osdep/os.h>

#define N_THREADS 8
#define M_ITERS   1000

/* Forwards (new tests) */
static void test_buf_alloc_free_concurrent(void **state);
static void test_buf_wait_alloc(void **state);

/* Define a fixed buffer pool: 8 buffers, data size 64, user_data 16 */
BT_BUF_POOL_DEFINE(test_pool, 16, 64, 16, NULL);

static int pool_prepare(void)
{
	/* do nothing */
	return 0;
}

static void test_alloc_len_fixed_and_with_data(void **state)
{
	(void)state;
	assert_int_equal(pool_prepare(), 0);

	struct bt_buf *b1 = bt_buf_alloc_len(&test_pool, 32, OS_TIMEOUT_NO_WAIT);
	assert_non_null(b1);
	assert_true(b1->size >= 32);
	assert_int_equal(b1->len, 0);

	struct bt_buf *b2 = bt_buf_alloc_fixed(&test_pool, OS_TIMEOUT_NO_WAIT);
	assert_non_null(b2);
	assert_int_equal(b2->len, 0);
	assert_int_equal(b2->size, test_pool.alloc->max_alloc_size);

	uint8_t ext[20];
	for (size_t i = 0; i < sizeof(ext); ++i) {
		ext[i] = (uint8_t)i;
	}
	struct bt_buf *b3 =
		bt_buf_alloc_with_data(&test_pool, ext, sizeof(ext), OS_TIMEOUT_NO_WAIT);
	assert_non_null(b3);
	assert_int_equal(b3->len, sizeof(ext));
	assert_true((b3->flags & BT_BUF_EXTERNAL_DATA) != 0);
	assert_ptr_equal(b3->data, ext);

	bt_buf_unref(b1);
	bt_buf_unref(b2);
	bt_buf_unref(b3);
}

static void test_reset_reserve_ref_unref_clone(void **state)
{
	(void)state;
	assert_int_equal(pool_prepare(), 0);

	struct bt_buf *b = bt_buf_alloc_len(&test_pool, 32, OS_TIMEOUT_NO_WAIT);
	assert_non_null(b);

	bt_buf_reserve(b, 8);
	assert_int_equal(bt_buf_headroom(b), 8);
	assert_int_equal(bt_buf_tailroom(b), b->size - 8);

	/* Add data */
	const uint8_t payload[] = {0xAA, 0xBB, 0xCC};
	bt_buf_add_mem(b, payload, sizeof(payload));
	assert_int_equal(b->len, (int)sizeof(payload));

	/* Ref/unref */
	struct bt_buf *r = bt_buf_ref(b);
	assert_ptr_equal(r, b);

	/* Clone: for fixed pools, clone should allocate new data and copy */
	struct bt_buf *c = bt_buf_clone(b, OS_TIMEOUT_NO_WAIT);
	assert_non_null(c);
	assert_int_equal(c->len, b->len);
	assert_memory_equal(c->data, b->data, b->len);
	/* user_data copied */
	memset(bt_buf_user_data(b), 0x5A, b->user_data_size);
	int rc = bt_buf_user_data_copy(c, b);
	assert_int_equal(rc, 0);
	assert_memory_equal(bt_buf_user_data(c), bt_buf_user_data(b), b->user_data_size);

	/* Unref both */
	bt_buf_unref(b); /* drops one ref */
	bt_buf_unref(b); /* drops to zero and returns to pool */
	bt_buf_unref(c);
}

static void test_frag_operations(void **state)
{
	(void)state;
	assert_int_equal(pool_prepare(), 0);
	struct bt_buf *head = bt_buf_alloc_len(&test_pool, 16, OS_TIMEOUT_NO_WAIT);
	struct bt_buf *frag1 = bt_buf_alloc_len(&test_pool, 8, OS_TIMEOUT_NO_WAIT);
	struct bt_buf *frag2 = bt_buf_alloc_len(&test_pool, 8, OS_TIMEOUT_NO_WAIT);
	assert_non_null(head);
	assert_non_null(frag1);
	assert_non_null(frag2);

	/* Fill data to identify */
	const uint8_t h[] = {1, 2, 3};
	const uint8_t f1[] = {4, 5};
	const uint8_t f2[] = {6, 7, 8};
	bt_buf_add_mem(head, h, sizeof(h));
	bt_buf_add_mem(frag1, f1, sizeof(f1));
	bt_buf_add_mem(frag2, f2, sizeof(f2));

	/* Add fragments */
	head = bt_buf_frag_add(head, frag1);
	bt_buf_frag_insert(bt_buf_frag_last(head), frag2);

	/* last */
	struct bt_buf *last = bt_buf_frag_last(head);
	assert_ptr_equal(last, frag2);

	/* linearize over fragments */
	uint8_t out[16] = {0};
	size_t copied = bt_buf_linearize(out, sizeof(out), head, 0, 16);
	assert_int_equal(copied, (int)(sizeof(h) + sizeof(f1) + sizeof(f2)));
	uint8_t expect[] = {1, 2, 3, 4, 5, 6, 7, 8};
	assert_memory_equal(out, expect, sizeof(expect));

	/* data_match starting at offset */
	const uint8_t sub[] = {5, 6, 7};
	size_t matched = bt_buf_data_match(head, 4, sub, sizeof(sub));
	assert_int_equal(matched, (int)sizeof(sub));

	/* Delete middle fragment */
	struct bt_buf *next = bt_buf_frag_del(head, frag1);
	assert_ptr_equal(next, frag2);

	/* Clean up (head still holds references to its frags) */
	bt_buf_unref(head);
}

static void test_append_bytes(void **state)
{
	(void)state;
	assert_int_equal(pool_prepare(), 0);
	struct bt_buf *b = bt_buf_alloc_len(&test_pool, 8, OS_TIMEOUT_NO_WAIT);
	assert_non_null(b);

	uint8_t longdata[24];
	for (size_t i = 0; i < sizeof(longdata); ++i) {
		longdata[i] = (uint8_t)i;
	}

	size_t added =
		bt_buf_append_bytes(b, sizeof(longdata), longdata, OS_TIMEOUT_FOREVER, NULL, NULL);
	/* May allocate additional fragments; verify total length */
	size_t total = bt_buf_frags_len(b);
	assert_int_equal(added, (int)sizeof(longdata));
	assert_int_equal(total, (int)sizeof(longdata));

	/* Clean */
	bt_buf_unref(b);
}

/* ===== Concurrent buf tests implementations ===== */

struct buf_alloc_arg {
	int succ;
};
static void buf_alloc_thread(void *arg)
{
	struct buf_alloc_arg *a = (struct buf_alloc_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		struct bt_buf *b = bt_buf_alloc_len(&test_pool, 16, OS_TIMEOUT_FOREVER);
		if (b) {
			bt_buf_unref(b);
			a->succ++;
		} else {
			(void)os_thread_yield();
		}
	}
}

static void test_buf_alloc_free_concurrent(void **state)
{
	(void)state;
	/* no pool_prepare here to avoid discarding initialized buffers */
	os_thread_t th[N_THREADS];
	struct buf_alloc_arg args[N_THREADS];
	memset(args, 0, sizeof(args));
	assert_int_equal(pool_prepare(), 0);
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_create(&th[i], buf_alloc_thread, &args[i], "buf_alloc",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	int total = 0;
	for (int i = 0; i < N_THREADS; ++i) {
		total += args[i].succ;
	}
	assert_true(total <= (int)(test_pool.buf_count * M_ITERS));

	/* Verify pool can still allocate some buffers */
	struct bt_buf *arr[64];
	size_t cnt = 0;
	for (size_t i = 0; i < test_pool.buf_count; ++i) {
		struct bt_buf *b = bt_buf_alloc_fixed(&test_pool, OS_TIMEOUT_NO_WAIT);
		if (!b) {
			break;
		}
		arr[cnt++] = b;
	}
	assert_true(cnt >= 0);
	assert_true(cnt <= (size_t)test_pool.buf_count);
	for (size_t i = 0; i < cnt; ++i) {
		bt_buf_unref(arr[i]);
	}
}

struct waiter_arg {
	struct bt_buf *out;
};
static void waiter_thread(void *arg)
{
	struct waiter_arg *wa = (struct waiter_arg *)arg;
	wa->out = NULL;
	for (int i = 0; i < 50; ++i) {
		struct bt_buf *b = bt_buf_alloc_len(&test_pool, 16, OS_TIMEOUT_FOREVER);
		if (b) {
			wa->out = b;
			return;
		}
		os_sleep_ms(5);
	}
}

static void test_buf_wait_alloc(void **state)
{
	(void)state;
	/* start waiter first */
	struct waiter_arg wa = {.out = NULL};
	os_thread_t wt;
	assert_int_equal(pool_prepare(), 0);
	assert_int_equal(os_thread_create(&wt, waiter_thread, &wa, "buf_waiter", OS_PRIORITY(0), 0),
			 0);

	/* Exhaust whatever buffers are currently available */
	struct bt_buf *held[64];
	size_t held_n = 0;
	for (size_t i = 0; i < test_pool.buf_count; ++i) {
		struct bt_buf *b = bt_buf_alloc_fixed(&test_pool, OS_TIMEOUT_FOREVER);
		if (!b) {
			break;
		}
		held[held_n++] = b;
	}

	/* Free one after 50ms to let waiter obtain it */
	os_sleep_ms(50);
	if (held_n > 0) {
		bt_buf_unref(held[held_n - 1]);
		held[held_n - 1] = NULL;
		held_n--;
	}
	assert_int_equal(os_thread_join(&wt, OS_TIMEOUT_FOREVER), 0);
	assert_non_null(wa.out);
	bt_buf_unref(wa.out);

	/* Free remaining */
	for (size_t i = 0; i < held_n; ++i) {
		if (held[i]) {
			bt_buf_unref(held[i]);
		}
	}

	/* Verify pool remains usable */
	struct bt_buf *arr[64];
	size_t cnt = 0;
	for (size_t i = 0; i < test_pool.buf_count; ++i) {
		struct bt_buf *b = bt_buf_alloc_fixed(&test_pool, OS_TIMEOUT_NO_WAIT);
		if (!b) {
			break;
		}
		arr[cnt++] = b;
	}
	assert_true(cnt <= (size_t)test_pool.buf_count);
	for (size_t i = 0; i < cnt; ++i) {
		bt_buf_unref(arr[i]);
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_alloc_len_fixed_and_with_data),
		cmocka_unit_test(test_reset_reserve_ref_unref_clone),
		cmocka_unit_test(test_frag_operations),
		cmocka_unit_test(test_append_bytes),
		cmocka_unit_test(test_buf_alloc_free_concurrent),
		cmocka_unit_test(test_buf_wait_alloc),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
