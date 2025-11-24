#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "base/queue/bt_queue.h"
#include "base/queue/bt_fifo.h"
#include "osdep/os.h"

static void test_basic_ops(void)
{
    struct bt_queue q;
    bt_queue_init(&q);
    assert(bt_queue_is_empty(&q) == true);

    int a = 1, b = 2;
    bt_queue_append(&q, &a);
    bt_queue_append(&q, &b);

    void *ph = bt_queue_peek_head(&q);
    void *pt = bt_queue_peek_tail(&q);
    assert(ph == &a);
    assert(pt == &b);

    void *x = bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
    assert(x == &a);
    x = bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
    assert(x == &b);
    x = bt_queue_get(&q, OS_TIMEOUT_NO_WAIT);
    assert(x == NULL);

    assert(bt_queue_is_empty(&q) == true);

    /* unique_append */
    bt_queue_append(&q, &a);
    bool ok = bt_queue_unique_append(&q, &a);
    assert(ok == false);
    assert(bt_queue_get(&q, OS_TIMEOUT_NO_WAIT) == &a);
}

static void test_remove(void)
{
    struct bt_queue q; bt_queue_init(&q);
    int a = 1, b = 2;
    bt_queue_append(&q, &a);
    bt_queue_append(&q, &b);
    /* remove existing */
    assert(bt_queue_remove(&q, &a) == true);
    /* removing again should fail */
    assert(bt_queue_remove(&q, &a) == false);
    /* remaining order */
    assert(bt_queue_get(&q, OS_TIMEOUT_NO_WAIT) == &b);
    assert(bt_queue_is_empty(&q) == true);
}

struct thread_args {
    struct bt_queue *q;
    void *out;
};

static void consumer_block(void *arg)
{
    struct thread_args *ta = (struct thread_args *)arg;
    void *v = bt_queue_get(ta->q, OS_TIMEOUT_FOREVER);
    ta->out = v;
}

static void test_blocking_get(void)
{
    struct bt_queue q;
    bt_queue_init(&q);

    struct thread_args ta;
    ta.q = &q; ta.out = NULL;

    os_thread_t thr;
    int rc = os_thread_create(&thr, consumer_block, &ta, "cons", 0, 0);
    assert(rc == 0);

    os_sleep_ms(50);
    static int value = 42;
    bt_queue_append(&q, &value);

    rc = os_thread_join(&thr, OS_TIMEOUT_FOREVER);
    assert(rc == 0);

    assert(ta.out == &value);
    assert(bt_queue_is_empty(&q) == true);
}

static void test_timeout(void)
{
    struct bt_queue q; bt_queue_init(&q);
    void *v = bt_queue_get(&q, OS_MSEC(100));
    assert(v == NULL);
}

/* Multi-producer / multi-consumer */
struct prod_args { struct bt_queue *q; int start; int count; };
struct cons_args { struct bt_queue *q; int total_to_consume; int consumed; };

static void producer_fn(void *arg)
{
    struct prod_args *pa = (struct prod_args *)arg;
    for (int i = 0; i < pa->count; i++) {
        int *p = (int *)os_malloc(sizeof(int));
        assert(p != NULL);
        *p = pa->start + i;
        bt_queue_append(pa->q, p);
    }
}

static void consumer_fn(void *arg)
{
    struct cons_args *ca = (struct cons_args *)arg;
    while (ca->consumed < ca->total_to_consume) {
        void *v = bt_queue_get(ca->q, OS_SECONDS(1)); /* 1s per loop */
        if (v == NULL) {
            /* Timed out: continue */
            continue;
        }
        ca->consumed++;
        os_free(v);
    }
}

static void test_multi_prod_cons(void)
{
    struct bt_queue q; bt_queue_init(&q);
    const int producers = 4;
    const int per_prod = 200; /* scalable workload */
    const int total = producers * per_prod;

    os_thread_t prod_thr[producers];
    struct prod_args pa[producers];

    for (int i = 0; i < producers; i++) {
        pa[i].q = &q; pa[i].start = i * per_prod; pa[i].count = per_prod;
        int rc = os_thread_create(&prod_thr[i], producer_fn, &pa[i], "prod", 0, 0);
        assert(rc == 0);
    }

    os_thread_t cons_thr[2];
    struct cons_args ca[2];
    for (int i = 0; i < 2; i++) {
        ca[i].q = &q; ca[i].total_to_consume = total / 2; ca[i].consumed = 0;
        int rc = os_thread_create(&cons_thr[i], consumer_fn, &ca[i], "cons", 0, 0);
        assert(rc == 0);
    }

    /* Join producers */
    for (int i = 0; i < producers; i++) {
        int rc = os_thread_join(&prod_thr[i], OS_TIMEOUT_FOREVER);
        assert(rc == 0);
    }

    /* Consumers should eventually consume all */
    for (int i = 0; i < 2; i++) {
        int rc = os_thread_join(&cons_thr[i], OS_TIMEOUT_FOREVER);
        assert(rc == 0);
    }

    int consumed_total = ca[0].consumed + ca[1].consumed;
    assert(consumed_total == total);
    assert(bt_queue_is_empty(&q) == true);
}

static void test_fifo_wrappers(void)
{
    struct bt_fifo f; bt_fifo_init(&f);
    int a=1,b=2,c=3;
    bt_fifo_put(&f, &a);
    bt_fifo_put(&f, &b);
    bt_fifo_put(&f, &c);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &a);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &b);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == &c);
    assert(bt_fifo_get(&f, OS_TIMEOUT_NO_WAIT) == NULL);
}

int main(void)
{
    printf("[buf_queue/test_queue] basic ops...\n");
    test_basic_ops();
    printf("[buf_queue/test_queue] remove...\n");
    test_remove();
    printf("[buf_queue/test_queue] blocking get...\n");
    test_blocking_get();
    printf("[buf_queue/test_queue] timeout...\n");
    test_timeout();
    printf("[buf_queue/test_queue] multi producer/consumer...\n");
    test_multi_prod_cons();
    printf("[buf_queue/test_queue] fifo wrappers...\n");
    test_fifo_wrappers();
    printf("[buf_queue/test_queue] all tests passed.\n");
    return 0;
}
