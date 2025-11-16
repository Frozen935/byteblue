#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdbool.h>
#include <ctype.h>

#include "base/log.h"

static enum stack_log_level config_stack_log_level = CONFIG_STACK_LOG_LEVEL;

bool bt_log_level_check(enum stack_log_level log_level)
{
	if (config_stack_log_level < log_level) {
		return false;
	}

	return true;
}

#ifdef __NuttX__
uint32_t bt_log_level_map(enum stack_log_level log_level)
{
	switch (log_level) {
	case LOG_LEVEL_NONE:
		return 0;
	case LOG_LEVEL_ERR:
		return LOG_ERR;
	case LOG_LEVEL_WRN:
		return LOG_WARNING;
	case LOG_LEVEL_INF:
		return LOG_INFO;
	case LOG_LEVEL_DBG:
		return LOG_DEBUG;
	default:
		return 0;
	}
}
#endif