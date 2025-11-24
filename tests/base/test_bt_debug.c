#include <stdint.h>
#include <string.h>
#include <stdarg.h>
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

#include <base/bt_debug.h>

static void call_vprint(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bt_debug_vprint(fmt, ap);
	va_end(ap);
}

static void test_bt_debug_prints(void **state)
{
	(void)state;
	bt_debug_print("debug %s %d", "msg", 42);

	/* vprint via helper */
	call_vprint("%s %d", "msg", 123);

	bt_debug_print("hello %s", "world");
	uint8_t buf[16];
	for (int i = 0; i < 16; ++i) {
		buf[i] = (uint8_t)i;
	}
	bt_debug_hexdump("hexdump", buf, sizeof(buf));
	/* No assertion: smoke test only, ensure no crash */
	assert_true(true);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_debug_prints),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
