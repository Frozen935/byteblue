#include <stdint.h>
#include <stdbool.h>
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

#include "osdep/os.h"

/* ----------------------- Semaphore ----------------------- */

static void test_sem_basic_and_limit(void **state)
{
	(void)state;
	os_sem_t sem;
	assert_int_equal(os_sem_init(&sem, 0, 2), 0);

	/* Non-blocking take should timeout */
	assert_int_equal(os_sem_take(&sem, OS_TIMEOUT_NO_WAIT), -ETIMEDOUT);

	/* Give twice, saturation at limit */
	assert_int_equal(os_sem_give(&sem), 0);
	assert_int_equal(os_sem_give(&sem), 0);
	unsigned int c = os_sem_count_get(&sem);
	assert_true(c <= 2);

	/* Third give should saturate (no effect) */
	(void)os_sem_give(&sem);
	unsigned int c2 = os_sem_count_get(&sem);
	assert_true(c2 == c || c2 == 2);

	/* Take twice succeeds */
	assert_int_equal(os_sem_take(&sem, OS_MSEC(10)), 0);
	assert_int_equal(os_sem_take(&sem, OS_MSEC(10)), 0);
	/* Now timeout */
	assert_int_equal(os_sem_take(&sem, OS_MSEC(10)), -ETIMEDOUT);
}

#define NUM_SEM_THREADS 4
struct sem_test_ctx {
	os_sem_t sem;
	volatile int workers_done;
};

static void sem_worker(void *arg)
{
	struct sem_test_ctx *ctx = (struct sem_test_ctx *)arg;
	/* Wait until the main thread gives the semaphore */
	assert_int_equal(os_sem_take(&ctx->sem, OS_TIMEOUT_FOREVER), 0);
	__sync_fetch_and_add(&ctx->workers_done, 1);
}

static void test_sem_multithreaded(void **state)
{
	(void)state;
	struct sem_test_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	os_thread_t threads[NUM_SEM_THREADS];

	/* Initialize semaphore with 0 count, so threads will block */
	assert_int_equal(os_sem_init(&ctx.sem, 0, 0), 0);

	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert_int_equal(os_thread_create(&threads[i], sem_worker, &ctx, "sem_w", 0, 0), 0);
	}

	/* Give threads a moment to start and block on the semaphore */
	os_sleep_ms(20);
	assert_int_equal(ctx.workers_done, 0);

	/* Unblock all threads */
	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert_int_equal(os_sem_give(&ctx.sem), 0);
	}

	/* Join all threads */
	for (int i = 0; i < NUM_SEM_THREADS; i++) {
		assert_int_equal(os_thread_join(&threads[i], OS_TIMEOUT_FOREVER), 0);
	}

	/* Check that all workers completed their task */
	assert_int_equal(ctx.workers_done, NUM_SEM_THREADS);
}

static void test_sem_reset_behavior(void **state)
{
	(void)state;
	os_sem_t sem;
	assert_int_equal(os_sem_init(&sem, 3, 5), 0);
	assert_true(os_sem_count_get(&sem) >= 3);
	assert_int_equal(os_sem_reset(&sem), 0);
	assert_int_equal(os_sem_count_get(&sem), 0);
}

/* ----------------------- Mutex ----------------------- */

static void test_mutex_lock_unlock_and_timed(void **state)
{
	(void)state;
	os_mutex_t m;
	assert_int_equal(os_mutex_init(&m), 0);

	/* Lock/unlock */
	assert_int_equal(os_mutex_lock(&m, OS_TIMEOUT_FOREVER), 0);
	assert_int_equal(os_mutex_unlock(&m), 0);

	/* Trylock should succeed when unlocked */
	assert_int_equal(os_mutex_lock(&m, OS_TIMEOUT_NO_WAIT), 0);

	/* Another timed trylock should timeout (already locked) */
	assert_int_equal(os_mutex_lock(&m, OS_TIMEOUT_NO_WAIT), -ETIMEDOUT);
	assert_int_equal(os_mutex_unlock(&m), 0);
}

static void test_mutex_timedlock_timeout(void **state)
{
	(void)state;
	os_mutex_t m;
	assert_int_equal(os_mutex_init(&m), 0);

	/* Lock the mutex, then attempt timed lock from same thread */
	assert_int_equal(os_mutex_lock(&m, OS_TIMEOUT_FOREVER), 0);
	int rc = os_mutex_lock(&m, OS_MSEC(10));
	/* Expect timeout on non-recursive mutex */
	assert_int_equal(rc, -ETIMEDOUT);
	assert_int_equal(os_mutex_unlock(&m), 0);
}

#define NUM_MUTEX_THREADS     4
#define INCREMENTS_PER_THREAD 1000

struct mutex_test_ctx {
	os_mutex_t mutex;
	volatile int counter;
};

static void mutex_worker(void *arg)
{
	struct mutex_test_ctx *ctx = (struct mutex_test_ctx *)arg;
	for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
		assert_int_equal(os_mutex_lock(&ctx->mutex, OS_TIMEOUT_FOREVER), 0);
		ctx->counter++;
		assert_int_equal(os_mutex_unlock(&ctx->mutex), 0);
	}
}

static void test_mutex_multithreaded(void **state)
{
	(void)state;
	struct mutex_test_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	os_thread_t threads[NUM_MUTEX_THREADS];

	assert_int_equal(os_mutex_init(&ctx.mutex), 0);

	for (int i = 0; i < NUM_MUTEX_THREADS; i++) {
		assert_int_equal(os_thread_create(&threads[i], mutex_worker, &ctx, "mtx_w", 0, 0),
				 0);
	}

	/* Join all threads */
	for (int i = 0; i < NUM_MUTEX_THREADS; i++) {
		assert_int_equal(os_thread_join(&threads[i], OS_TIMEOUT_FOREVER), 0);
	}

	/* Verify the final counter value to check for race conditions */
	int expected = NUM_MUTEX_THREADS * INCREMENTS_PER_THREAD;
	assert_int_equal(ctx.counter, expected);
}

/* ----------------------- Condition variable ----------------------- */

struct cond_ctx {
	os_mutex_t m;
	os_cond_t c;
	bool ready;
};

static void cond_signaler(void *arg)
{
	struct cond_ctx *ctx = (struct cond_ctx *)arg;
	os_sleep_ms(20);
	assert_int_equal(os_mutex_lock(&ctx->m, OS_TIMEOUT_FOREVER), 0);
	ctx->ready = true;
	assert_int_equal(os_cond_signal(&ctx->c), 0);
	assert_int_equal(os_mutex_unlock(&ctx->m), 0);
}

static void test_cond_wait_signal_timeout(void **state)
{
	(void)state;
	struct cond_ctx ctx;
	assert_int_equal(os_mutex_init(&ctx.m), 0);
	assert_int_equal(os_cond_init(&ctx.c), 0);
	ctx.ready = false;

	assert_int_equal(os_mutex_lock(&ctx.m, OS_TIMEOUT_FOREVER), 0);
	/* Start signaler thread */
	os_thread_t thr;
	assert_int_equal(os_thread_create(&thr, cond_signaler, &ctx, "sig", 0, 0), 0);

	/* Wait with timeout longer than signaler delay */
	assert_int_equal(os_cond_wait(&ctx.c, &ctx.m, OS_MSEC(100)), 0);
	assert_true(ctx.ready == true);
	assert_int_equal(os_mutex_unlock(&ctx.m), 0);
	assert_int_equal(os_thread_join(&thr, OS_TIMEOUT_FOREVER), 0);

	/* Test timeout when not signaled */
	assert_int_equal(os_mutex_lock(&ctx.m, OS_TIMEOUT_FOREVER), 0);
	ctx.ready = false;
	/* No signal -> timeout */
	assert_int_equal(os_cond_wait(&ctx.c, &ctx.m, OS_MSEC(20)), -ETIMEDOUT);
	assert_int_equal(os_mutex_unlock(&ctx.m), 0);
}

struct bc_ctx {
	os_mutex_t m;
	os_cond_t c;
	bool ready;
	int woke;
};

static void cond_waiter(void *arg)
{
	struct bc_ctx *ctx = (struct bc_ctx *)arg;
	assert_int_equal(os_mutex_lock(&ctx->m, OS_TIMEOUT_FOREVER), 0);
	while (!ctx->ready) {
		int rc = os_cond_wait(&ctx->c, &ctx->m, OS_TIMEOUT_FOREVER);
		assert_int_equal(rc, 0);
	}
	ctx->woke++;
	assert_int_equal(os_mutex_unlock(&ctx->m), 0);
}

static void test_cond_broadcast_multiple_waiters(void **state)
{
	(void)state;
	struct bc_ctx ctx;
	assert_int_equal(os_mutex_init(&ctx.m), 0);
	assert_int_equal(os_cond_init(&ctx.c), 0);
	ctx.ready = false;
	ctx.woke = 0;

	os_thread_t ws[3];
	for (int i = 0; i < 3; i++) {
		assert_int_equal(os_thread_create(&ws[i], cond_waiter, &ctx, "wait", 0, 0), 0);
	}

	os_sleep_ms(20);
	assert_int_equal(os_mutex_lock(&ctx.m, OS_TIMEOUT_FOREVER), 0);
	ctx.ready = true;
	assert_int_equal(os_cond_broadcast(&ctx.c), 0);
	assert_int_equal(os_mutex_unlock(&ctx.m), 0);

	for (int i = 0; i < 3; i++) {
		assert_int_equal(os_thread_join(&ws[i], OS_TIMEOUT_FOREVER), 0);
	}

	assert_int_equal(ctx.woke, 3);
}

/* ----------------------- Thread ----------------------- */

static void write_worker(void *arg)
{
	int *p = (int *)arg;
	*p = 42;
}

static void test_thread_create_join_name(void **state)
{
	(void)state;
	os_thread_t thr;
	int v = 0;
	assert_int_equal(os_thread_create(&thr, write_worker, &v, "worker", 0, 0), 0);
	int rcname = os_thread_name_set(&thr, "worker");
	assert_true(rcname == 0 || rcname == -EINVAL);
	assert_int_equal(os_thread_join(&thr, OS_TIMEOUT_FOREVER), 0);
	assert_int_equal(v, 42);
}

static void sleepy_worker(void *arg)
{
	(void)arg;
	/* Sleep long enough to be canceled */
	for (int i = 0; i < 10; i++) {
		os_sleep_ms(10);
	}
}

static void test_thread_cancel_behavior(void **state)
{
	(void)state;
	os_thread_t thr;
	assert_int_equal(os_thread_create(&thr, sleepy_worker, NULL, "sleepy", 0, 0), 0);
	os_sleep_ms(10);
	assert_int_equal(os_thread_cancel(&thr), 0);
	/* Join should succeed after cancellation */
	assert_int_equal(os_thread_join(&thr, OS_TIMEOUT_FOREVER), 0);
}

static void is_current_worker(void *arg)
{
	os_thread_t *thr = (os_thread_t *)arg;
	/* Inside this thread, it must be current */
	assert_true(os_thread_is_current(thr));
}

static void test_thread_self_and_is_current(void **state)
{
	(void)state;
	/* os_thread_self should return a non-zero id */
	os_tid_t self = os_thread_self();
	assert_true(self != 0);

	os_thread_t thr;
	assert_int_equal(os_thread_create(&thr, is_current_worker, &thr, "selfchk", 0, 0), 0);
	/* In main thread, thr is not current */
	assert_false(os_thread_is_current(&thr));
	assert_int_equal(os_thread_join(&thr, OS_TIMEOUT_FOREVER), 0);
}

/* ----------------------- Scheduler/Critical ----------------------- */

static void test_sched_lock_unlock_and_critical(void **state)
{
	(void)state;
	/* Simply exercise the calls and ensure they don't deadlock */
	os_sched_lock();
	os_sched_unlock();

	os_enter_critical();
	os_exit_critical();
}

/* ----------------------- Timer ----------------------- */

static volatile int timer_hits = 0;

static void timer_cb(os_timer_t *t, void *arg)
{
	(void)t;
	(void)arg;
	timer_hits++;
}

static void test_timer_start_stop_remaining(void **state)
{
	(void)state;
	timer_hits = 0;
	os_timer_t t;
	assert_int_equal(os_timer_create(&t, timer_cb, NULL), 0);
	assert_int_equal(os_timer_start(&t, 30), 0);
	/* Wait until callback fires */
	for (int i = 0; i < 12 && timer_hits == 0; i++) {
		os_sleep_ms(5);
	}
	assert_true(timer_hits >= 1);
	uint64_t rem = os_timer_remaining_ms(&t);
	assert_true(rem == 0 || rem < 30);
	assert_int_equal(os_timer_stop(&t), 0);
	assert_int_equal(os_timer_delete(&t), 0);
}

static void timer_nohit_cb(os_timer_t *t, void *arg)
{
	(void)t;
	volatile int *hits = (volatile int *)arg;
	(*hits)++;
}

static void test_timer_stop_before_expiry(void **state)
{
	(void)state;
	volatile int hits = 0;
	os_timer_t t;
	assert_int_equal(os_timer_create(&t, timer_nohit_cb, (void *)&hits), 0);
	assert_int_equal(os_timer_start(&t, 200), 0);
	/* Immediately stop */
	assert_int_equal(os_timer_stop(&t), 0);
	/* Short wait to ensure callback does not fire */
	os_sleep_ms(50);
	assert_int_equal(hits, 0);
	assert_int_equal(os_timer_delete(&t), 0);
}

/* ----------------------- Time ----------------------- */

static void test_time_sleep_monotonic(void **state)
{
	(void)state;
	uint64_t t0 = os_time_get_ms();
	os_sleep_ms(10);
	uint64_t t1 = os_time_get_ms();
	assert_true(t1 > t0);
}

/* ----------------------- Memory ----------------------- */

static void test_mem_alloc_free(void **state)
{
	(void)state;
	void *p = os_malloc(128);
	assert_non_null(p);
	memset(p, 0xAB, 128);
	os_free(p);

	void *q = os_calloc(4, 32);
	assert_non_null(q);
	uint8_t *u = (uint8_t *)q;
	for (int i = 0; i < 128; i++) {
		assert_int_equal(u[i], 0);
	}
	os_free(q);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		/* Semaphore */
		cmocka_unit_test(test_sem_basic_and_limit),
		cmocka_unit_test(test_sem_multithreaded),
		cmocka_unit_test(test_sem_reset_behavior),
		/* Mutex */
		cmocka_unit_test(test_mutex_lock_unlock_and_timed),
		cmocka_unit_test(test_mutex_timedlock_timeout),
		cmocka_unit_test(test_mutex_multithreaded),
		/* Condition variable */
		cmocka_unit_test(test_cond_wait_signal_timeout),
		cmocka_unit_test(test_cond_broadcast_multiple_waiters),
		/* Threads */
		cmocka_unit_test(test_thread_create_join_name),
		cmocka_unit_test(test_thread_cancel_behavior),
		cmocka_unit_test(test_thread_self_and_is_current),
		/* Scheduler/Critical */
		cmocka_unit_test(test_sched_lock_unlock_and_critical),
		/* Timer */
		cmocka_unit_test(test_timer_start_stop_remaining),
		cmocka_unit_test(test_timer_stop_before_expiry),
		/* Time */
		cmocka_unit_test(test_time_sleep_monotonic),
		/* Memory */
		cmocka_unit_test(test_mem_alloc_free),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
