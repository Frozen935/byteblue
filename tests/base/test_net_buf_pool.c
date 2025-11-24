#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "base/bt_buf.h"

/* Define a fixed-size bt_buf pool: 8 buffers, data size 64 bytes, user data 16 bytes */
BT_BUF_POOL_DEFINE(my_pool, 8, 64, 16, NULL);

static void test_alloc_and_id(void)
{
    struct bt_buf *b = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    assert(b != NULL);
    int id = bt_buf_id(b);
    assert(id >= 0 && id < 8);

    /* Reserve headroom and add/pull data */
    bt_buf_reserve(b, 8);
    assert(bt_buf_headroom(b) == 8);
    assert(bt_buf_tailroom(b) == 56);

    const uint8_t data[20] = {0};
    bt_buf_add_mem(b, data, sizeof(data));
    assert(b->len == 20);

    /* Push header and pull it back */
    uint8_t hdr[4] = {1,2,3,4};
    bt_buf_push_mem(b, hdr, sizeof(hdr));
    assert(b->len == 24);
    uint8_t *p = bt_buf_pull_mem(b, 4);
    assert(p[0]==1 && p[1]==2 && p[2]==3 && p[3]==4);

    /* Remove from tail */
    bt_buf_remove_mem(b, 10);
    assert(b->len == 10);

    /* Ref/unref returns to pool */
    bt_buf_ref(b);
    bt_buf_unref(b); /* ref -> 1 */
    bt_buf_unref(b); /* ref -> 0 and back to pool */

    struct bt_buf *b2 = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    assert(b2 != NULL);
    /* It's possible (but not guaranteed) to get the same pointer back */
    bt_buf_unref(b2);
}

static void test_clone_and_slist(void)
{
    struct bt_buf *b = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    assert(b);
    bt_buf_add_u8(b, 0xAA);
    bt_buf_add_le16(b, 0xBEEF);

    struct bt_buf *c = bt_buf_clone(b, OS_TIMEOUT_NO_WAIT);
    assert(c);
    assert(c->len == b->len);
    /* user data copy */
    memset(bt_buf_user_data(b), 0x33, b->user_data_size);
    assert(bt_buf_user_data_copy(c, b) == 0);

    /* slist put/get order */
    bt_slist_t list; bt_slist_init(&list);
    bt_buf_slist_put(&list, b);
    bt_buf_slist_put(&list, c);
    struct bt_buf *g1 = bt_buf_slist_get(&list);
    struct bt_buf *g2 = bt_buf_slist_get(&list);
    assert(g1 == b);
    assert(g2 == c);

    bt_buf_unref(b);
    bt_buf_unref(c);
}

static void test_frag_and_linearize(void)
{
    struct bt_buf *head = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    struct bt_buf *f1   = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    struct bt_buf *f2   = bt_buf_alloc(&my_pool, OS_TIMEOUT_NO_WAIT);
    assert(head && f1 && f2);

    const uint8_t a[10] = {0,1,2,3,4,5,6,7,8,9};
    const uint8_t b[8]  = {10,11,12,13,14,15,16,17};
    const uint8_t c[6]  = {18,19,20,21,22,23};
    bt_buf_add_mem(head, a, sizeof(a));
    bt_buf_add_mem(f1, b, sizeof(b));
    bt_buf_add_mem(f2, c, sizeof(c));

    bt_buf_frag_add(head, f1);
    bt_buf_frag_add(head, f2);
    assert(bt_buf_frags_len(head) == (sizeof(a)+sizeof(b)+sizeof(c)));

    uint8_t out[24] = {0};
    size_t copied = bt_buf_linearize(out, sizeof(out), head, 0, sizeof(out));
    assert(copied == sizeof(out));
    for (int i = 0; i < 24; i++) {
        assert(out[i] == (uint8_t)i);
    }

    /* match */
    size_t m = bt_buf_data_match(head, 5, &out[5], 10);
    assert(m == 10);

    /* delete mid frag */
    (void)bt_buf_frag_del(head, f1);
    assert(bt_buf_frags_len(head) == (sizeof(a)+sizeof(c)));

    bt_buf_unref(head);
    /* f1 freed by frag_del, f2 still chained to head->frags and freed when head unref above */
}

int main(void)
{
    printf("[buf_queue/test_bt_buf_pool] alloc/id...\n");
    test_alloc_and_id();
    printf("[buf_queue/test_bt_buf_pool] clone + slist...\n");
    test_clone_and_slist();
    printf("[buf_queue/test_bt_buf_pool] frag + linearize + match...\n");
    test_frag_and_linearize();
    printf("[buf_queue/test_bt_buf_pool] all tests passed.\n");
    return 0;
}
