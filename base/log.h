#ifndef __BASE_LOG_H__
#define __BASE_LOG_H__

#ifndef LOG_TAG
#define LOG_TAG "blue"
#endif

#include <base/bt_debug.h>

#define 	LOG_LEVEL_NONE  0
#define 	LOG_LEVEL_ERR   1
#define 	LOG_LEVEL_WRN   2
#define 	LOG_LEVEL_INF   3
#define 	LOG_LEVEL_DBG   4

/* Log level constants to satisfy switch/array usage */
enum stack_log_level {
	STACK_LOG_LEVEL_NONE = LOG_LEVEL_NONE,
	STACK_LOG_LEVEL_ERR = LOG_LEVEL_ERR,
	STACK_LOG_LEVEL_WRN = LOG_LEVEL_WRN,
	STACK_LOG_LEVEL_INF = LOG_LEVEL_INF,
	STACK_LOG_LEVEL_DBG = LOG_LEVEL_DBG
};
#define LOG_EN 1

/* ===== Logging macros (safe no-op) ===== */

#ifndef CONFIG_STACK_LOG_LEVEL
#define CONFIG_STACK_LOG_LEVEL LOG_LEVEL_INF
#endif

#if LOG_EN
#define __S_LINE__  UTILS_STRINGIFY(__LINE__)

extern bool bt_log_level_check(enum stack_log_level log_level);

#define LOG_IMPL(level, fmt, ...)                                                                  \
	bt_debug_print("[%d][%s:%4d] %s: " fmt "\n", level, LOG_TAG, __LINE__, __func__, ##__VA_ARGS__);

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_DBG
#define LOG_DBG(fmt, ...)                                                                          \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_DBG)) {                                        \
			LOG_IMPL(LOG_LEVEL_DBG, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_DBG(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_INF
#define LOG_INF(fmt, ...)                                                                          \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_INF)) {                                        \
			LOG_IMPL(LOG_LEVEL_INF, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_INF(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_WRN
#define LOG_WRN(fmt, ...)                                                                          \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_WRN)) {                                        \
			LOG_IMPL(LOG_LEVEL_WRN, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_WRN(fmt, ...) /* no-op */
#endif

#if CONFIG_STACK_LOG_LEVEL >= LOG_LEVEL_ERR
#define LOG_ERR(fmt, ...)                                                                          \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_ERR)) {                                        \
			LOG_IMPL(LOG_LEVEL_ERR, fmt, ##__VA_ARGS__);                               \
		}                                                                                  \
	} while (0)
#else
#define LOG_ERR(fmt, ...) /* no-op */
#endif

#define LOG_HEXDUMP_DBG(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_DBG)) {                                        \
			LOG_DBG(fmt, ##__VA_ARGS__);                                               \
			bt_debug_hexdump(NULL, buf, len);                                               \
		}                                                                                  \
	} while (0)
#define LOG_HEXDUMP_INF(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_INF)) {                                        \
			LOG_INF(fmt, ##__VA_ARGS__);                                               \
			bt_debug_hexdump(NULL, buf, len);                                               \
		}                                                                                  \
	} while (0)
#define LOG_HEXDUMP_WRN(buf, len, fmt, ...)                                                        \
	do {                                                                                       \
		if (bt_log_level_check(LOG_LEVEL_WRN)) {                                        \
			LOG_WRN(fmt, ##__VA_ARGS__);                                               \
			bt_debug_hexdump(NULL, buf, len);                                               \
		}                                                                                  \
	} while (0)

#else
#define LOG_DBG(fmt, ...)                   /* no-op */
#define LOG_INF(fmt, ...)                   /* no-op */
#define LOG_WRN(fmt, ...)                   /* no-op */
#define LOG_ERR(fmt, ...)                   /* no-op */
#define LOG_HEXDUMP_DBG(buf, len, fmt, ...) /* no-op */
#define LOG_HEXDUMP_INF(buf, len, fmt, ...) /* no-op */
#define LOG_HEXDUMP_WRN(buf, len, fmt, ...) /* no-op */
#endif                                      /* !LOG_EN */

#endif /* __BASE_LOG_H__ */