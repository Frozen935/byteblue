#include <stdint.h>
#include <stdlib.h>
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

#include <base/queue/bt_queue.h>
#include <utils/bt_slist.h>
#include <osdep/os.h>

#define N_THREADS 8
#define M_ITERS   1000

/* Forwards */
static void test_queue_producers_consumers(void **state);
static void test_queue_unique_append_concurrent(void **state);

struct item {
	bt_snode_t node;
	int value;
};

static void test_queue_basic_ops(void **state)
{
	(void)state;
	struct bt_queue q;
	bt_queue_init(&q);
	assert_true(bt_queue_is_empty(&q));

	struct item a = {.value = 1}, b = {.value = 2}, c = {.value = 3};

	bt_queue_append(&q, &a);
	bt_queue_prepend(&q, &b);
	bt_queue_append(&q, &c);

	/* peek head/tail */
	struct item *head = (struct item *)bt_queue_peek_head(&q);
	struct item *tail = (struct item *)bt_queue_peek_tail(&q);
	assert_non_null(head);
	assert_non_null(tail);
	assert_int_equal(head->value, 2); /* b at head */
	assert_int_equal(tail->value, 3); /* c at tail */

	/* get no-wait */
	struct item *g1 = (struct item *)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
	assert_non_null(g1);
	assert_int_equal(g1->value, 2);

	/* timed wait when queue empty -> returns NULL after timeout */
	bt_queue_cancel_wait(&q);                   /* ensure no blocking leftover */
	(void)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT); /* consume a */
	(void)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT); /* consume c */
	assert_true(bt_queue_is_empty(&q));
	void *gnull = bt_queue_get(&q, OS_MSEC(5));
	assert_null(gnull);

	/* unique_append */
	bt_queue_append(&q, &a);
	bool uniq = bt_queue_unique_append(&q, &a);
	assert_false(uniq);
	uniq = bt_queue_unique_append(&q, &b);
	assert_true(uniq);

	/* remove specific */
	bool removed = bt_queue_remove(&q, &a);
	assert_true(removed);
	removed = bt_queue_remove(&q, &a);
	assert_false(removed);
}

/* ===== Concurrent queue tests ===== */
struct prod_item {
	bt_snode_t node;
	int prod;
	int seq;
};
struct prod_arg {
	struct bt_queue *q;
	struct prod_item *items;
	int prod;
};
static void producer_thread(void *arg)
{
	struct prod_arg *pa = (struct prod_arg *)arg;
	for (int i = 0; i < M_ITERS; ++i) {
		pa->items[i].prod = pa->prod;
		pa->items[i].seq = i;
		bt_queue_append(pa->q, &pa->items[i]);
	}
}

struct cons_arg {
	struct bt_queue *q;
	int total;
	int seen[N_THREADS];
};
static void consumer_thread(void *arg)
{
	struct cons_arg *ca = (struct cons_arg *)arg;
	int consumed = 0;
	while (consumed < ca->total) {
		struct prod_item *it = (struct prod_item *)bt_queue_get(ca->q, OS_TIMEOUT_FOREVER);
		if (!it) {
			continue; /* should not happen with FOREVER */
		}
		/* verify per-producer order monotonic */
		int p = it->prod;
		assert_true(it->seq >= ca->seen[p]);
		ca->seen[p] = it->seq;
		consumed++;
	}
}

static void test_queue_producers_consumers(void **state)
{
	(void)state;
	struct bt_queue q;
	bt_queue_init(&q);
	os_thread_t prod_th[N_THREADS];
	struct prod_item items[N_THREADS][M_ITERS];
	memset(items, 0, sizeof(items));
	struct prod_arg pargs[N_THREADS];
	for (int p = 0; p < N_THREADS; ++p) {
		pargs[p].q = &q;
		pargs[p].items = items[p];
		pargs[p].prod = p;
		assert_int_equal(os_thread_create(&prod_th[p], producer_thread, &pargs[p], "prod",
						  OS_PRIORITY(0), 0),
				 0);
	}
	struct cons_arg carg;
	memset(&carg, 0, sizeof(carg));
	carg.q = &q;
	carg.total = N_THREADS * M_ITERS;
	os_thread_t cons_th;
	assert_int_equal(
		os_thread_create(&cons_th, consumer_thread, &carg, "cons", OS_PRIORITY(0), 0), 0);
	for (int p = 0; p < N_THREADS; ++p) {
		assert_int_equal(os_thread_join(&prod_th[p], OS_TIMEOUT_FOREVER), 0);
	}
	assert_int_equal(os_thread_join(&cons_th, OS_TIMEOUT_FOREVER), 0);
	/* queue should be empty */
	assert_true(bt_queue_is_empty(&q));

	/* Prepend order test (single-thread) */
	struct item a = {.value = 11}, b = {.value = 22}, c = {.value = 33};
	bt_queue_init(&q);
	bt_queue_prepend(&q, &a);
	bt_queue_prepend(&q, &b);
	bt_queue_prepend(&q, &c);
	struct item *g1 = (struct item *)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
	struct item *g2 = (struct item *)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
	struct item *g3 = (struct item *)bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
	assert_int_equal(g1->value, 33);
	assert_int_equal(g2->value, 22);
	assert_int_equal(g3->value, 11);
}

struct uniq_arg {
	struct bt_queue *q;
	bt_snode_t *node;
	int succ;
};
static void uniq_thread(void *arg)
{
	struct uniq_arg *ua = (struct uniq_arg *)arg;
	bool ok = bt_queue_unique_append(ua->q, ua->node);
	ua->succ += ok ? 1 : 0;
}

static void test_queue_unique_append_concurrent(void **state)
{
	(void)state;
	struct bt_queue q;
	bt_queue_init(&q);
	bt_snode_t shared;
	memset(&shared, 0, sizeof(shared));
	os_thread_t th[N_THREADS];
	struct uniq_arg args[N_THREADS];
	memset(args, 0, sizeof(args));
	for (int i = 0; i < N_THREADS; ++i) {
		args[i].q = &q;
		args[i].node = &shared;
		assert_int_equal(
			os_thread_create(&th[i], uniq_thread, &args[i], "uniq", OS_PRIORITY(0), 0),
			0);
	}
	int total = 0;
	for (int i = 0; i < N_THREADS; ++i) {
		assert_int_equal(os_thread_join(&th[i], OS_TIMEOUT_FOREVER), 0);
		total += args[i].succ;
	}
	assert_int_equal(total, 1);
	/* cleanup */
	(void)bt_queue_remove(&q, &shared);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_queue_basic_ops),
		cmocka_unit_test(test_queue_producers_consumers),
		cmocka_unit_test(test_queue_unique_append_concurrent),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
