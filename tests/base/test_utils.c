#include <stdint.h>
#include <string.h>
#include <stdio.h>
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

#include <base/utils.h>

static void test_hex_roundtrip(void **state)
{
	(void)state;
	uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	char hex[16] = {0};
	size_t n = bin2hex(data, sizeof(data), hex, sizeof(hex));
	assert_int_equal(n, 8);
	assert_string_equal(hex, "deadbeef");

	uint8_t out[4] = {0};
	size_t m = hex2bin("de ad:be-ef", strlen("de ad:be-ef"), out, sizeof(out));
	assert_int_equal(m, sizeof(out));
	assert_memory_equal(out, data, sizeof(data));

	uint8_t x;
	assert_int_equal(char2hex('f', &x), 0);
	assert_int_equal(x, 15);
	char c;
	assert_int_equal(hex2char(0xA, &c), 0);
	assert_int_equal(c, 'a');
	assert_int_equal(char2hex('x', &x), -EINVAL);
	assert_int_equal(hex2char(20, &c), -EINVAL);
}

static void test_crc16_reflect(void **state)
{
	(void)state;
	const char *s = "123456789";
	/* Compute crc twice and verify determinism and non-zero */
	uint16_t crc1 = crc16_reflect(0xA001, 0xFFFF, (const uint8_t *)s, strlen(s));
	uint16_t crc2 = crc16_reflect(0xA001, 0xFFFF, (const uint8_t *)s, strlen(s));
	assert_int_equal(crc1, crc2);
	assert_true(crc1 != 0);
}

static void test_crc32_ieee_and_incremental(void **state)
{
	(void)state;
	const uint8_t part1[] = {'1', '2', '3', '4', '5'};
	const uint8_t part2[] = {'6', '7', '8', '9'};
	const uint8_t all[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

	uint32_t full = crc32_ieee(all, sizeof(all));
	assert_int_equal(full, 0xCBF43926);

	uint32_t incr = crc32_ieee_update(0x0, part1, sizeof(part1));
	incr = crc32_ieee_update(incr, part2, sizeof(part2));
	assert_int_equal(incr, full);
}

static void test_u8_to_dec_and_eq_helpers(void **state)
{
	(void)state;
	char buf[8] = {0};
	uint8_t d = u8_to_dec(buf, sizeof(buf), 0);
	assert_int_equal(d, 1);
	assert_string_equal(buf, "0");

	memset(buf, 0, sizeof(buf));
	d = u8_to_dec(buf, sizeof(buf), 123);
	assert_int_equal(d, 3);
	assert_string_equal(buf, "123");

	uint8_t a[] = {1, 2, 3};
	uint8_t b[] = {1, 2, 3};
	uint8_t diff[] = {1, 2, 4};
	assert_true(util_memeq(a, b, sizeof(a)));
	assert_false(util_memeq(a, diff, sizeof(a)));
	assert_true(util_eq(a, sizeof(a), b, sizeof(b)));
	assert_false(util_eq(a, sizeof(a), diff, sizeof(diff)));
}

static void test_utf8_helpers(void **state)
{
	(void)state;
	/* Valid UTF-8: "Hello" + U+00A9 (2-byte) + U+20AC (3-byte) */
	const char *src = "Hello\xC2\xA9\xE2\x82\xAC";
	char dst[32];
	utf8_lcpy(dst, src, sizeof(dst));
	assert_string_equal(dst, src);
	int count = utf8_count_chars(dst);
	assert_int_equal(count, 7);

	/* Invalid sequence counting */
	assert_int_equal(utf8_count_chars("\xC3("), -EINVAL);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_hex_roundtrip),
		cmocka_unit_test(test_crc16_reflect),
		cmocka_unit_test(test_crc32_ieee_and_incremental),
		cmocka_unit_test(test_u8_to_dec_and_eq_helpers),
		cmocka_unit_test(test_utf8_helpers),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
