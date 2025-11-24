#include <stdint.h>
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

#include <base/log.h>

static void test_bt_log_level_check_and_macros(void **state)
{
	(void)state;
	/* Default CONFIG_STACK_LOG_LEVEL is LOG_LEVEL_INF (3) unless overridden */
	assert_true(bt_log_level_check(LOG_LEVEL_ERR));
	assert_true(bt_log_level_check(LOG_LEVEL_INF));
	/* DBG may be disabled depending on config; allow either */
	__maybe_unused bool dbg_ok = bt_log_level_check(LOG_LEVEL_DBG);

	LOG_ERR("error log %d", 1);
	LOG_WRN("warn log %d", 2);
	LOG_INF("info log %d", 3);
	LOG_DBG("debug log %d (enabled=%d)", 4, dbg_ok ? 1 : 0);

	assert_true(true); /* smoke */
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_bt_log_level_check_and_macros),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
