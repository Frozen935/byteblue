#ifndef __BASE_ASSERT_H__
#define __BASE_ASSERT_H__

#include <assert.h>

#define ASSERT_EN 1

#if ASSERT_EN
/* ===== Assert macros ===== */
#define __ASSERT_NO_MSG(test)                                                                      \
	do {                                                                                       \
		if (!(test)) {                                                                     \
			assert(test);                                                              \
		}                                                                                  \
	} while (false)

#define __ASSERT_MSG(test, msg, ...)                                                               \
	do {                                                                                       \
		if (!(test)) {                                                                     \
			LOG_ERR(msg, ##__VA_ARGS__);                                               \
			assert(test);                                                              \
		}                                                                                  \
	} while (false)

#define __ASSERT_PRINT(fmt, ...)                                                                   \
	do {                                                                                       \
		LOG_INF(fmt, ##__VA_ARGS__);                                                       \
		assert(0);                                                                         \
	} while (false)

#else
#define __ASSERT_NO_MSG(cond)
#define __ASSERT_MSG(cond, msg, ...)
#define __ASSERT_PRINT(...)
#endif
#endif /* __BASE_ASSERT_H__ */