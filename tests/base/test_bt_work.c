#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
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

#include <base/bt_work.h>
#include <osdep/os.h>

#define N_THREADS 8
#define M_ITERS   1000

/* Forwards */
static void test_work_submit_concurrent(void **state);
static void test_delayable_concurrent(void **state);

static atomic_int g_count;

static void simple_handler(struct bt_work *work)
{
	(void)work;
	atomic_fetch_add(&g_count, 1);
}

static void delay_handler(struct bt_work *work)
{
	(void)work;
	atomic_fetch_add(&g_count, 10);
}

static void test_work_queue_submit_and_flush(void **state)
{
	(void)state;
	atomic_store(&g_count, 0);

	struct bt_work_q q;
	bt_work_queue_init(&q);
	const struct bt_work_queue_config cfg = {
		.name = "testq",
		.no_yield = false,
	};
	bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

	struct bt_work w;
	bt_work_init(&w, simple_handler);

	int rc = bt_work_submit_to_queue(&q, &w);
	assert_true(rc > 0);

	/* Allow handler to run */
	os_sleep_ms(20);
	assert_int_equal(atomic_load(&g_count), 1);
	/* Work should be idle or transitioning; just verify handler ran */
	(void)bt_work_busy_get(&w);

	/* Flush an already completed work: should return false */
	struct bt_work_sync sync;
	bool flushed = bt_work_flush(&w, &sync);
	assert_false(flushed);

	/* Cancel non-pending work */
	rc = bt_work_cancel(&w);
	assert_true(rc >= 0);

	/* Drain and stop queue */
	rc = bt_work_queue_drain(&q, true);
	assert_int_equal(rc, 0);
	rc = bt_work_queue_stop(&q, OS_TIMEOUT_FOREVER);
	assert_int_equal(rc, 0);
}

static void test_delayable_schedule_reschedule_and_cancel(void **state)
{
	(void)state;
	atomic_store(&g_count, 0);

	struct bt_work_q q;
	bt_work_queue_init(&q);
	const struct bt_work_queue_config cfg = {
		.name = "testq2",
		.no_yield = false,
	};
	bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

	struct bt_work_delayable dw;
	bt_work_init_delayable(&dw, delay_handler);

	/* Schedule after 10ms */
	int rc = bt_work_schedule_for_queue(&q, &dw, OS_MSEC(10));
	assert_true(rc > 0);
	os_sleep_ms(30);
	assert_int_equal(atomic_load(&g_count), 10);

	/* Reschedule again (no delay) to run immediately */
	rc = bt_work_reschedule_for_queue(&q, &dw, OS_TIMEOUT_NO_WAIT);
	assert_true(rc > 0);
	os_sleep_ms(10);
	assert_true(atomic_load(&g_count) >= 20);

	/* Cancel sync when not pending should be no-op */
	struct bt_work_sync sync;
	bool pending = bt_work_cancel_delayable_sync(&dw, &sync);
	assert_false(pending);

	/* Flush delayable (not pending) */
	bool need_flush = bt_work_flush_delayable(&dw, &sync);
	assert_false(need_flush);

	/* Plug, drain and stop */
	rc = bt_work_queue_drain(&q, true);
	assert_int_equal(rc, 0);
	rc = bt_work_queue_stop(&q, OS_TIMEOUT_FOREVER);
	assert_int_equal(rc, 0);
}

/* ===== Concurrent work tests ===== */
#define K_WORKS (32)
struct wsubmit_arg {
	struct bt_work_q *q;
	struct bt_work *works;
	int start;
	int count;
};
static void submit_thread(void *arg)
{
	struct wsubmit_arg *wa = (struct wsubmit_arg *)arg;
	for (int i = 0; i < wa->count; ++i) {
		int idx = wa->start + i;
		int rc = bt_work_submit_to_queue(wa->q, &wa->works[idx]);
		assert_true(rc > 0);
	}
}

static void test_work_submit_concurrent(void **state)
{
	(void)state;
	atomic_store(&g_count, 0);
	struct bt_work_q q;
	bt_work_queue_init(&q);
	const struct bt_work_queue_config cfg = {.name = "mtq", .no_yield = false};
	bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

	struct bt_work works[K_WORKS];
	for (int i = 0; i < K_WORKS; ++i) {
		bt_work_init(&works[i], simple_handler);
	}
	os_thread_t th[N_THREADS];
	struct wsubmit_arg args[N_THREADS];
	int per = K_WORKS / N_THREADS;
	int rem = K_WORKS % N_THREADS;
	int pos = 0;
	for (int i = 0; i < N_THREADS; ++i) {
		int take = per + (i < rem ? 1 : 0);
		args[i].q = &q;
		args[i].works = works;
		args[i].start = pos;
		args[i].count = take;
		assert_int_equal(os_thread_create(&th[i], submit_thread, &args[i], "wsub",
						  OS_PRIORITY(0), 0),
				 0);
		pos += take;
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	os_sleep_ms(50);
	assert_int_equal(atomic_load(&g_count), K_WORKS);
	assert_int_equal(bt_work_queue_drain(&q, true), 0);
	assert_int_equal(bt_work_queue_stop(&q, OS_TIMEOUT_FOREVER), 0);
}

#define K_DELAY (24)
struct schedule_arg {
	struct bt_work_q *q;
	struct bt_work_delayable *dw;
	int start;
	int count;
};
static void schedule_thread(void *arg)
{
	struct schedule_arg *sa = (struct schedule_arg *)arg;
	for (int i = 0; i < sa->count; ++i) {
		int idx = sa->start + i;
		os_timeout_t dly = (idx % 3 == 0) ? OS_MSEC(10) : OS_MSEC(50);
		int rc = bt_work_schedule_for_queue(sa->q, &sa->dw[idx], dly);
		assert_true(rc > 0);
		/* cancel some long-delay ones immediately */
		if (dly == OS_MSEC(50) && (idx % 2 == 0)) {
			struct bt_work_sync sync;
			(void)bt_work_cancel_delayable_sync(&sa->dw[idx], &sync);
		}
	}
}

static void test_delayable_concurrent(void **state)
{
	(void)state;
	atomic_store(&g_count, 0);
	struct bt_work_q q;
	bt_work_queue_init(&q);
	const struct bt_work_queue_config cfg = {.name = "mtq2", .no_yield = false};
	bt_work_queue_start(&q, 2048, OS_PRIORITY(0), &cfg);

	struct bt_work_delayable dw[K_DELAY];
	for (int i = 0; i < K_DELAY; ++i) {
		bt_work_init_delayable(&dw[i], delay_handler);
	}
	os_thread_t th[N_THREADS];
	struct schedule_arg args[N_THREADS];
	int per = K_DELAY / N_THREADS;
	int rem = K_DELAY % N_THREADS;
	int pos = 0;
	for (int i = 0; i < N_THREADS; ++i) {
		int take = per + (i < rem ? 1 : 0);
		args[i].q = &q;
		args[i].dw = dw;
		args[i].start = pos;
		args[i].count = take;
		assert_int_equal(os_thread_create(&th[i], schedule_thread, &args[i], "sched",
						  OS_PRIORITY(0), 0),
				 0);
		pos += take;
	}
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
	}
	/* Allow short-delay ones to run */
	os_sleep_ms(80);
	/* Count expected executed: those with 10ms delay (idx % 3 == 0) */
	int expected = 0;
	for (int i = 0; i < K_DELAY; ++i) {
		if ((i % 3) == 0) {
			expected++;
		}
	}
	/* Some long-delay were cancelled; ensure at least short delays ran */
	int got = atomic_load(&g_count);
	assert_true(got >= expected * 10);
	assert_true(got <= K_DELAY * 10);

	/* Flush remaining (no pending) and stop */
	for (int i = 0; i < K_DELAY; ++i) {
		struct bt_work_sync sync;
		(void)bt_work_flush_delayable(&dw[i], &sync);
	}
	assert_int_equal(bt_work_queue_drain(&q, true), 0);
	assert_int_equal(bt_work_queue_stop(&q, OS_TIMEOUT_FOREVER), 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_work_queue_submit_and_flush),
		cmocka_unit_test(test_delayable_schedule_reschedule_and_cancel),
		cmocka_unit_test(test_work_submit_concurrent),
		cmocka_unit_test(test_delayable_concurrent),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
