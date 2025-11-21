#ifndef __INCLUDE_UTIL_MACRO_H__
#define __INCLUDE_UTIL_MACRO_H__

#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC 1000U
#endif
#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000U
#endif
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000U
#endif
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000U
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

#ifndef BIT64
#define BIT64(_n) (1ULL << (_n))
#endif

#ifndef BIT_MASK
#define BIT_MASK(n) (BIT(n) - 1UL)
#endif

#ifndef BIT64_MASK
#define BIT64_MASK(n) (BIT64(n) - 1ULL)
#endif

#define BITS_PER_LONG      (BITS_PER_BYTE * sizeof(long))
#define BITS_PER_LONG_LONG (BITS_PER_BYTE * sizeof(long long))

#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#ifndef WRITE_BIT
#define WRITE_BIT(var, bit, set) ((var) = (set) ? ((var) | BIT(bit)) : ((var) & ~BIT(bit)))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef IN_RANGE
#define IN_RANGE(val, min, max) ((val) >= (min) && (val) <= (max))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

#define _XXXX1 _YYYY,

#define __IS_ENABLED3(ignore_this, val, ...) val
#define __IS_ENABLED2(one_or_two_args)       __IS_ENABLED3(one_or_two_args 1, 0)
#define __IS_ENABLED1(config_macro)          __IS_ENABLED2(_XXXX##config_macro)
#define IS_ENABLED(config_macro)             __IS_ENABLED1(config_macro)

#define __DEBRACKET(...)                            __VA_ARGS__
#define __GET_ARG2_DEBRACKET(ignore_this, val, ...) __DEBRACKET val
#define __COND_CODE_X(one_or_two_args, _if_code, _else_code)                                       \
	__GET_ARG2_DEBRACKET(one_or_two_args _if_code, _else_code)
#define __COND_CODE_1(_flag, _if_1_code, _else_code)                                               \
	__COND_CODE_X(_XXXX##_flag, _if_1_code, _else_code)

#define UTIL_COND_CODE(_flag, _if_1_code, _else_code) __COND_CODE_1(_flag, _if_1_code, _else_code)

#define IF_ENABLED(_flag, _code)  UTIL_COND_CODE(_flag, _code, ())
#define IF_DISABLED(_flag, _code) UTIL_COND_CODE(_flag, (), _code)

#define __FIELD_LSB_GET(value)  ((value) & -(value))
#define FIELD_GET(mask, value)  (((value) & (mask)) / __FIELD_LSB_GET(mask))
#define FIELD_PREP(mask, value) (((value) * __FIELD_LSB_GET(mask)) & (mask))

#define __UTIL_PRIMITIVE_CAT(a, ...)   a##__VA_ARGS__
#define __UTIL_CAT(a, ...)             __UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_LISTIFY(LEN, F, sep, ...) __UTIL_CAT(UTIL_LISTIFY_, LEN)(F, sep, __VA_ARGS__)

#define __UTIL_DO_CONCAT(x, y) x##y
#define UTIL_CONCAT(x, y)      __UTIL_DO_CONCAT(x, y)

#define __UTILS_STRINGIFY(x) #x
#define UTILS_STRINGIFY(s) __UTILS_STRINGIFY(s)

#endif /* __INCLUDE_UTIL_MACRO_H__ */