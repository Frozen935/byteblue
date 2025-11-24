#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>
#include <string.h>

#if defined(__has_include)
#if __has_include(<cmocka.h>)
#include <cmocka.h>
#else
#include <cmocka.h>
#endif
#else
#include <cmocka.h>
#endif

#include <base/bt_atomic.h>
#include <osdep/os.h>

#define N_THREADS 8
#define M_ITERS   1000

/* Forward declarations for new concurrent tests */
static void test_atomic_inc_concurrent(void **state);
static void test_atomic_add_sub_concurrent(void **state);
static void test_atomic_cas_increment_concurrent(void **state);
static void test_atomic_bitmask_concurrent(void **state);
static void test_atomic_ptr_concurrent(void **state);

static void test_bt_atomic_get_set(void **state)
{
	(void)state;
	bt_atomic_t v = 0UL;
	assert_int_equal(bt_atomic_get(&v), 0UL);
	bt_atomic_val_t old = bt_atomic_set(&v, 123UL);
	assert_int_equal(old, 0UL);
	assert_int_equal(bt_atomic_get(&v), 123UL);

	/* NULL safety */
	assert_int_equal(bt_atomic_get(NULL), 0UL);
	assert_int_equal(bt_atomic_set(NULL, 42UL), 0UL);
}

static void test_bt_atomic_inc_dec_add_sub(void **state)
{
	(void)state;
	bt_atomic_t v = 5UL;
	bt_atomic_val_t old = bt_atomic_inc(&v);
	assert_int_equal(old, 5UL);
	assert_int_equal(bt_atomic_get(&v), 6UL);

	old = bt_atomic_dec(&v);
	assert_int_equal(old, 6UL);
	assert_int_equal(bt_atomic_get(&v), 5UL);

	bt_atomic_add(&v, 7UL);
	assert_int_equal(bt_atomic_get(&v), 12UL);

	bt_atomic_sub(&v, 20UL);
	assert_int_equal(bt_atomic_get(&v), (bt_atomic_val_t)(12UL - 20UL));
}

static void test_bt_atomic_cas(void **state)
{
	(void)state;
	bt_atomic_t v = 1UL;
	assert_true(bt_atomic_cas(&v, 1UL, 7UL));
	assert_int_equal(bt_atomic_get(&v), 7UL);
	assert_false(bt_atomic_cas(&v, 1UL, 9UL));
	assert_int_equal(bt_atomic_get(&v), 7UL);

	/* NULL safety */
	assert_false(bt_atomic_cas(NULL, 0UL, 1UL));
}

static void test_bt_atomic_bit_helpers(void **state)
{
	(void)state;
	ATOMIC_DEFINE(bitmap, 64);
	/* All clear initially */
	memset(bitmap, 0, sizeof(bitmap));
	for (int i = 0; i < 64; i++) {
		assert_false(bt_atomic_test_bit(bitmap, i));
	}

	/* Negative bit safe */
	assert_false(bt_atomic_test_bit(bitmap, -1));
	bt_atomic_set_bit(bitmap, -1); /* should be no-op */

	/* Set and test some bits */
	bt_atomic_set_bit(bitmap, 0);
	bt_atomic_set_bit(bitmap, 7);
	bt_atomic_set_bit(bitmap, 63);
	assert_true(bt_atomic_test_bit(bitmap, 0));
	assert_true(bt_atomic_test_bit(bitmap, 7));
	assert_true(bt_atomic_test_bit(bitmap, 63));
	assert_false(bt_atomic_test_bit(bitmap, 8));

	/* Clear bit */
	bt_atomic_clear_bit(bitmap, 7);
	assert_false(bt_atomic_test_bit(bitmap, 7));

	/* test_and_set semantics */
	bool was_set = bt_atomic_test_and_set_bit(bitmap, 0);
	assert_true(was_set); /* was already set */
	was_set = bt_atomic_test_and_set_bit(bitmap, 8);
	assert_false(was_set);
	assert_true(bt_atomic_test_bit(bitmap, 8));

	/* test_and_clear semantics */
	bool was_set2 = bt_atomic_test_and_clear_bit(bitmap, 8);
	assert_true(was_set2);
	assert_false(bt_atomic_test_bit(bitmap, 8));

	/* set_bit_to */
	bt_atomic_set_bit_to(bitmap, 10, true);
	assert_true(bt_atomic_test_bit(bitmap, 10));
	bt_atomic_set_bit_to(bitmap, 10, false);
	assert_false(bt_atomic_test_bit(bitmap, 10));
}

static void test_bt_atomic_ptr_helpers(void **state)
{
	(void)state;
	int x = 123, y = 456;
	bt_atomic_ptr_t p = NULL;
	assert_ptr_equal(bt_atomic_ptr_get(&p), NULL);
	assert_ptr_equal(bt_atomic_ptr_set(&p, &x), NULL);
	assert_ptr_equal(bt_atomic_ptr_get(&p), &x);

	assert_true(bt_atomic_ptr_cas(&p, &x, &y));
	assert_ptr_equal(bt_atomic_ptr_get(&p), &y);
	assert_false(bt_atomic_ptr_cas(&p, &x, &y));

	assert_ptr_equal(bt_atomic_ptr_clear(&p), &y);
	assert_ptr_equal(bt_atomic_ptr_get(&p), NULL);
}

/* ===== Concurrent atomic tests implementations ===== */

struct inc_arg {
	bt_atomic_t *val;
};

static void inc_thread(void *arg)
{
	struct inc_arg *a = (struct inc_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		(void)bt_atomic_inc(a->val);
	}
}

static void test_atomic_inc_concurrent(void **state)
{
	(void)state;
	bt_atomic_t v = 0UL;
	os_thread_t th[N_THREADS];
	struct inc_arg args[N_THREADS];
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].val = &v;
		assert_int_equal(
			os_thread_create(&th[i], inc_thread, &args[i], "inc", OS_PRIORITY(0), 0),
			0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	assert_int_equal(bt_atomic_get(&v), (bt_atomic_val_t)(N_THREADS * M_ITERS));
}

struct addsub_arg {
	bt_atomic_t *val;
	bool add;
};
static void addsub_thread(void *arg)
{
	struct addsub_arg *a = (struct addsub_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		if (a->add) {
			bt_atomic_add(a->val, 1UL);
		} else {
			bt_atomic_sub(a->val, 1UL);
		}
	}
}

static void test_atomic_add_sub_concurrent(void **state)
{
	(void)state;
	bt_atomic_t v = 0UL;
	os_thread_t th[N_THREADS];
	struct addsub_arg args[N_THREADS];
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].val = &v;
		args[i].add = (i < (N_THREADS / 2));
		assert_int_equal(os_thread_create(&th[i], addsub_thread, &args[i], "addsub",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	assert_int_equal(bt_atomic_get(&v), 0UL);
}

struct casinc_arg {
	bt_atomic_t *val;
};
static void casinc_thread(void *arg)
{
	struct casinc_arg *a = (struct casinc_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		while (1) {
			bt_atomic_val_t old = bt_atomic_get(a->val);
			bt_atomic_val_t neu = old + 1UL;
			if (bt_atomic_cas(a->val, old, neu)) {
				break;
			}
		}
	}
}

static void test_atomic_cas_increment_concurrent(void **state)
{
	(void)state;
	bt_atomic_t v = 0UL;
	os_thread_t th[N_THREADS];
	struct casinc_arg args[N_THREADS];
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].val = &v;
		assert_int_equal(os_thread_create(&th[i], casinc_thread, &args[i], "casinc",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	assert_int_equal(bt_atomic_get(&v), (bt_atomic_val_t)(N_THREADS * M_ITERS));
}

struct bitmap_arg {
	bt_atomic_t *bitmap;
	int start_bit;
	int count;
	bool clear_phase;
};
static void bitmap_thread(void *arg)
{
	struct bitmap_arg *a = (struct bitmap_arg *)arg;
	for (int i = 0; i < a->count; ++i) {
		int bit = a->start_bit + i;
		if (!a->clear_phase) {
			bt_atomic_set_bit(a->bitmap, bit);
		} else {
			/* clear every 4th bit within range */
			if ((i % 4) == 0) {
				bt_atomic_clear_bit(a->bitmap, bit);
			}
		}
	}
}

static void test_atomic_bitmask_concurrent(void **state)
{
	(void)state;
	ATOMIC_DEFINE(bitmap, 128);
	memset(bitmap, 0, sizeof(bitmap));
	os_thread_t th[N_THREADS];
	struct bitmap_arg args[N_THREADS];
	int per = 128 / N_THREADS;
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].bitmap = bitmap;
		args[i].start_bit = i * per;
		args[i].count = per;
		args[i].clear_phase = false;
		assert_int_equal(os_thread_create(&th[i], bitmap_thread, &args[i], "bmset",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	/* verify all set */
	for (int b = 0; b < 128; ++b) {
		assert_true(bt_atomic_test_bit(bitmap, b));
	}
	/* clear phase */
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].clear_phase = true;
		assert_int_equal(os_thread_create(&th[i], bitmap_thread, &args[i], "bmclr",
						  OS_PRIORITY(0), 0),
				 0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	/* verify every 4th bit cleared */
	for (int i = 0; i < N_THREADS; ++i) {
		int start = i * per;
		for (int j = 0; j < per; ++j) {
			int bit = start + j;
			if ((j % 4) == 0) {
				assert_false(bt_atomic_test_bit(bitmap, bit));
			} else {
				assert_true(bt_atomic_test_bit(bitmap, bit));
			}
		}
	}
}

struct ptr_arg {
	bt_atomic_ptr_t *pp;
	void *a;
	void *b;
	int *succ;
};
static void ptr_thread(void *arg)
{
	struct ptr_arg *p = (struct ptr_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		/* toggle pointer via CAS loop, count success for each toggle */
		while (1) {
			void *cur = bt_atomic_ptr_get(p->pp);
			void *next = (cur == p->a) ? p->b : p->a;
			if (bt_atomic_ptr_cas(p->pp, cur, next)) {
				(*p->succ)++;
				break;
			}
		}
	}
}

static void test_atomic_ptr_concurrent(void **state)
{
	(void)state;
	int xa = 1, xb = 2;
	bt_atomic_ptr_t p = NULL;
	(void)bt_atomic_ptr_set(&p, &xa);
	os_thread_t th[N_THREADS];
	int succ_counts[N_THREADS];
	memset(succ_counts, 0, sizeof(succ_counts));
	struct ptr_arg args[N_THREADS];
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].pp = &p;
		args[i].a = &xa;
		args[i].b = &xb;
		args[i].succ = &succ_counts[i];
		assert_int_equal(
			os_thread_create(&th[i], ptr_thread, &args[i], "ptr", OS_PRIORITY(0), 0),
			0);
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	/* total success equals total toggles */
	int total = 0;
	for (int i = 0; i < N_THREADS; ++i) {
		total += succ_counts[i];
	}
	assert_int_equal(total, N_THREADS * M_ITERS);
	/* final pointer must equal initial (even toggles) */
	assert_ptr_equal(bt_atomic_ptr_get(&p), &xa);
}

static int setup(void **state)
{
	(void)state;
	return 0;
}
static int teardown(void **state)
{
	(void)state;
	return 0;
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_bt_atomic_get_set, setup, teardown),
		cmocka_unit_test_setup_teardown(test_bt_atomic_inc_dec_add_sub, setup, teardown),
		cmocka_unit_test_setup_teardown(test_bt_atomic_cas, setup, teardown),
		cmocka_unit_test_setup_teardown(test_bt_atomic_bit_helpers, setup, teardown),
		cmocka_unit_test_setup_teardown(test_bt_atomic_ptr_helpers, setup, teardown),
		cmocka_unit_test_setup_teardown(test_atomic_inc_concurrent, setup, teardown),
		cmocka_unit_test_setup_teardown(test_atomic_add_sub_concurrent, setup, teardown),
		cmocka_unit_test_setup_teardown(test_atomic_cas_increment_concurrent, setup,
						teardown),
		cmocka_unit_test_setup_teardown(test_atomic_bitmask_concurrent, setup, teardown),
		cmocka_unit_test_setup_teardown(test_atomic_ptr_concurrent, setup, teardown),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}