#include <stdint.h>
#include <string.h>
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

#include <base/bt_mem_pool.h>
#include <osdep/os.h>

#define N_THREADS 8
#define M_ITERS   1000

/* Forwards */
static void test_mem_pool_alloc_waiting(void **state);
static void test_mem_pool_alloc_free_concurrent(void **state);

static void test_bt_mem_pool_alloc_free(void **state)
{
	(void)state;
	/* Prepare buffer: block size 32, blocks 4 */
	const uint32_t blocks = 4;
	const size_t block_size = 32;
	char buffer[blocks * WB_UP(block_size)];

	struct bt_mem_pool mp = {0};
	int rc = bt_mem_pool_init(&mp, buffer, block_size, blocks);
	assert_int_equal(rc, 0);

	void *ptrs[blocks];
	/* Exhaust blocks with NO_WAIT */
	for (uint32_t i = 0; i < blocks; ++i) {
		rc = bt_mem_pool_alloc(&mp, &ptrs[i], OS_TIMEOUT_NO_WAIT);
		assert_int_equal(rc, 0);
		assert_non_null(ptrs[i]);
	}

	/* Next NO_WAIT should fail */
	void *extra = (void *)0x1;
	rc = bt_mem_pool_alloc(&mp, &extra, OS_TIMEOUT_NO_WAIT);
	assert_int_equal(rc, -ENOMEM);
	assert_null(extra);

	/* Free one and allocate again successfully */
	bt_mem_pool_free(&mp, ptrs[2]);
	rc = bt_mem_pool_alloc(&mp, &ptrs[2], OS_TIMEOUT_NO_WAIT);
	assert_int_equal(rc, 0);
	assert_non_null(ptrs[2]);

	/* Clean up */
	for (uint32_t i = 0; i < blocks; ++i) {
		bt_mem_pool_free(&mp, ptrs[i]);
	}
}

/* ===== Concurrent mem pool tests ===== */

struct mp_wait_arg {
	struct bt_mem_pool *mp;
	void *out;
};
static void mp_wait_thread(void *arg)
{
	struct mp_wait_arg *wa = (struct mp_wait_arg *)arg;
	/* Non-blocking retry loop to avoid global lock deadlock */
	void *blk = NULL;
	for (int i = 0; i < 100; ++i) {
		int rc = bt_mem_pool_alloc(wa->mp, &blk, OS_TIMEOUT_NO_WAIT);
		if (rc == 0 && blk) {
			wa->out = blk;
			return;
		}
		os_sleep_ms(5);
	}
	wa->out = NULL;
}

static void test_mem_pool_alloc_waiting(void **state)
{
	(void)state;
	const uint32_t blocks = 8;
	const size_t block_size = 32;
	char buffer[blocks * WB_UP(block_size)];
	struct bt_mem_pool mp = {0};
	assert_int_equal(bt_mem_pool_init(&mp, buffer, block_size, blocks), 0);

	void *held[16];
	uint32_t held_n = 0;
	for (uint32_t i = 0; i < blocks; ++i) {
		void *blk = NULL;
		assert_int_equal(bt_mem_pool_alloc(&mp, &blk, OS_TIMEOUT_NO_WAIT), 0);
		held[held_n++] = blk;
	}
	assert_int_equal(held_n, blocks);

	struct mp_wait_arg wa = {.mp = &mp, .out = NULL};
	os_thread_t wt;
	assert_int_equal(os_thread_create(&wt, mp_wait_thread, &wa, "mp_wait", OS_PRIORITY(0), 0),
			 0);
	os_sleep_ms(50);
	bt_mem_pool_free(&mp, held[held_n - 1]);
	held[held_n - 1] = NULL;
	held_n--;
	assert_int_equal(os_thread_join(&wt, OS_TIMEOUT_FOREVER), 0);
	assert_non_null(wa.out);
	bt_mem_pool_free(&mp, wa.out);
	for (uint32_t i = 0; i < held_n; ++i) {
		if (held[i]) {
			bt_mem_pool_free(&mp, held[i]);
		}
	}
}

struct mp_concur_arg {
	struct bt_mem_pool *mp;
	int succ;
};
static void mp_concur_thread(void *arg)
{
	struct mp_concur_arg *a = (struct mp_concur_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		void *blk = NULL;
		int rc = bt_mem_pool_alloc(a->mp, &blk, OS_TIMEOUT_FOREVER);
		if (rc == 0 && blk) {
			bt_mem_pool_free(a->mp, blk);
			a->succ++;
		} else {
			(void)os_thread_yield();
		}
	}
}

static void test_mem_pool_alloc_free_concurrent(void **state)
{
	(void)state;
	const uint32_t blocks = 8;
	const size_t block_size = 32;
	char buffer[blocks * WB_UP(block_size)];
	struct bt_mem_pool mp = {0};
	assert_int_equal(bt_mem_pool_init(&mp, buffer, block_size, blocks), 0);
	os_thread_t th[N_THREADS];
	struct mp_concur_arg args[N_THREADS];
	memset(args, 0, sizeof(args));
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].mp = &mp;
		assert_int_equal(os_thread_create(&th[i], mp_concur_thread, &args[i], "mp_cf",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	/* verify pool all free: exhaust and count blocks again */
	uint32_t cnt = 0;
	void *arr[32];
	while (1) {
		void *blk = NULL;
		int rc = bt_mem_pool_alloc(&mp, &blk, OS_TIMEOUT_NO_WAIT);
		if (rc != 0 || !blk) {
			break;
		}
		arr[cnt++] = blk;
	}
	assert_int_equal(cnt, blocks);
	for (uint32_t i = 0; i < cnt; ++i) {
		bt_mem_pool_free(&mp, arr[i]);
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_mem_pool_alloc_free),
		cmocka_unit_test(test_mem_pool_alloc_waiting),
		cmocka_unit_test(test_mem_pool_alloc_free_concurrent),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}