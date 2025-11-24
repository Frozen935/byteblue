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

static void test_init_reserve_head_tail_max(void **state)
{
	(void)state;
	struct bt_buf_simple *b = BT_BUF_SIMPLE(64);
	bt_buf_simple_init(b, 0);
	assert_int_equal(bt_buf_simple_headroom(b), 0);
	assert_int_equal(bt_buf_simple_tailroom(b), 64);
	assert_int_equal(bt_buf_simple_max_len(b), 64);

	bt_buf_simple_reset(b);
	bt_buf_simple_reserve(b, 16);
	assert_int_equal(bt_buf_simple_headroom(b), 16);
	assert_int_equal(bt_buf_simple_tailroom(b), 64 - 16);
	assert_int_equal(bt_buf_simple_max_len(b), 64 - 16);
}

static void test_add_push_pull_remove(void **state)
{
	(void)state;
	struct bt_buf_simple *b = BT_BUF_SIMPLE(32);
	bt_buf_simple_init(b, 8);

	/* add_mem and add_u8 */
	const uint8_t seq[] = {1, 2, 3, 4, 5};
	bt_buf_simple_add_mem(b, seq, sizeof(seq));
	assert_int_equal(b->len, sizeof(seq));
	*bt_buf_simple_add_u8(b, 99) = 99;
	assert_int_equal(b->len, sizeof(seq) + 1);

	/* remove_u8 */
	uint8_t last = bt_buf_simple_remove_u8(b);
	assert_int_equal(last, 99);
	assert_int_equal(b->len, sizeof(seq));

	/* push_mem at head */
	const uint8_t head[] = {7, 8};
	bt_buf_simple_push_mem(b, head, sizeof(head));
	assert_int_equal(b->len, sizeof(seq) + sizeof(head));
	assert_memory_equal(b->data, head, sizeof(head));

	/* pull_mem */
	uint8_t tmp[2] = {0};
	memcpy(tmp, bt_buf_simple_pull_mem(b, sizeof(tmp)), sizeof(tmp));
	assert_int_equal(tmp[0], 7);
	assert_int_equal(tmp[1], 8);
	assert_int_equal(b->len, sizeof(seq));
}

static void test_endian_add_remove(void **state)
{
	(void)state;
	struct bt_buf_simple *b = BT_BUF_SIMPLE(128);
	bt_buf_simple_init(b, 0);

	/* LE/BE 16/24/32/40/48/64 add & remove */
	bt_buf_simple_add_le16(b, 0x1234);
	bt_buf_simple_add_be16(b, 0x5678);
	bt_buf_simple_add_le24(b, 0x00A1B2);
	bt_buf_simple_add_be24(b, 0x00C3D4);
	bt_buf_simple_add_le32(b, 0x89ABCDEF);
	bt_buf_simple_add_be32(b, 0x10203040);
	bt_buf_simple_add_le40(b, 0x0102030405ULL);
	bt_buf_simple_add_be40(b, 0x0A0B0C0D0EULL);
	bt_buf_simple_add_le48(b, 0x111213141516ULL);
	bt_buf_simple_add_be48(b, 0x212223242526ULL);
	bt_buf_simple_add_le64(b, 0x3132333435363738ULL);
	bt_buf_simple_add_be64(b, 0x4142434445464748ULL);

	/* Remove in reverse order */
	assert_int_equal(bt_buf_simple_remove_be64(b), 0x4142434445464748ULL);
	assert_int_equal(bt_buf_simple_remove_le64(b), 0x3132333435363738ULL);
	assert_int_equal(bt_buf_simple_remove_be48(b), 0x212223242526ULL);
	assert_int_equal(bt_buf_simple_remove_le48(b), 0x111213141516ULL);
	assert_int_equal(bt_buf_simple_remove_be40(b), 0x0A0B0C0D0EULL);
	assert_int_equal(bt_buf_simple_remove_le40(b), 0x0102030405ULL);
	assert_int_equal(bt_buf_simple_remove_be32(b), 0x10203040U);
	assert_int_equal(bt_buf_simple_remove_le32(b), 0x89ABCDEFU);
	assert_int_equal(bt_buf_simple_remove_be24(b), 0x00C3D4U);
	assert_int_equal(bt_buf_simple_remove_le24(b), 0x00A1B2U);
	assert_int_equal(bt_buf_simple_remove_be16(b), 0x5678U);
	assert_int_equal(bt_buf_simple_remove_le16(b), 0x1234U);

	assert_int_equal(b->len, 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_init_reserve_head_tail_max),
		cmocka_unit_test(test_add_push_pull_remove),
		cmocka_unit_test(test_endian_add_remove),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
