#ifndef __BASE_UTILS_INTERNAL_H__
#define __BASE_UTILS_INTERNAL_H__

/* Misc helpers for build-only: hex2bin, crc16_reflect */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <utils/bt_utils.h>

#ifndef CHECKIF
#define CHECKIF(expr) if (expr)
#endif

#define IS_BIT_SET(value, bit) ((((value) >> (bit)) & (0x1)) != 0)

#ifndef CODE_UNREACHABLE
#define CODE_UNREACHABLE                                                                           \
	do {                                                                                       \
		assert(0);                                                                         \
	} while (0)
#endif

#ifndef __fallthrough
#define __fallthrough /* no-op */
#endif

#define TIMEOUT_EQ(a, b) ((a) == (b))

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#ifndef POINTER_TO_UINT
#define POINTER_TO_UINT(x) ((uintptr_t)(x))
#endif

#ifndef UINT_TO_POINTER
#define UINT_TO_POINTER(x) ((void *)(uintptr_t)(x))
#endif
#ifndef POINTER_TO_INT
#define POINTER_TO_INT(x) ((intptr_t)(x))
#endif
#ifndef INT_TO_POINTER
#define INT_TO_POINTER(x) ((void *)(intptr_t)(x))
#endif

#ifndef ARRAY_FOR_EACH
#define ARRAY_FOR_EACH(array, idx) for (size_t idx = 0; (idx) < ARRAY_SIZE(array); ++(idx))
#endif

#ifndef ARRAY_FOR_EACH_PTR
#define ARRAY_FOR_EACH_PTR(array, ptr)                                                             \
	for (__typeof__(*(array)) *ptr = (array); (size_t)((ptr) - (array)) < ARRAY_SIZE(array);   \
	     ++(ptr))
#endif

#ifndef IS_ARRAY_ELEMENT
#define IS_ARRAY_ELEMENT(array, ptr)                                                               \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                                \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]) &&                    \
	 (POINTER_TO_UINT(ptr) - POINTER_TO_UINT(array)) % sizeof((array)[0]) == 0)
#endif

#ifndef ARRAY_INDEX
#define ARRAY_INDEX(array, ptr)                                                                    \
	({                                                                                         \
		__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(array, ptr));                                     \
		(__typeof__((array)[0]) *)(ptr) - (array);                                         \
	})
#endif

#ifndef PART_OF_ARRAY
#define PART_OF_ARRAY(array, ptr)                                                                  \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                                \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]))
#endif

#ifndef CLAMP
#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/* misc helpers used by classic */
#ifndef NUM_BITS
#define NUM_BITS(sz) ((int)((sz) * 8))
#endif

#ifndef FLEXIBLE_ARRAY_DECLARE
#define FLEXIBLE_ARRAY_DECLARE(type, name)                                                         \
	struct {                                                                                   \
		struct {                                                                           \
		} __unused_##name;                                                                 \
		type name[];                                                                       \
	}
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, align)                                                                         \
	((((unsigned long)(x) + ((unsigned long)(align) - 1)) / (unsigned long)(align)) *          \
	 (unsigned long)(align))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)                                                                       \
	(((unsigned long)(x) / (unsigned long)(align)) * (unsigned long)(align))
#endif

#define WB_UP(x) ROUND_UP(x, sizeof(void *))

/* TODO: deprecated, use dynamic register instead */
#ifndef STRUCT_SECTION_ITERABLE
#define STRUCT_SECTION_ITERABLE(struct_type, _name)                                                \
       struct struct_type _name
#endif

#ifndef STRUCT_SECTION_FOREACH
#define STRUCT_SECTION_FOREACH(struct_type, var) for (struct struct_type *var = NULL; false;)
#endif

int char2hex(char c, uint8_t *x);
int hex2char(uint8_t x, char *c);
size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen);
size_t hex2bin(const char *hex, size_t hex_len, uint8_t *out, size_t out_size);
uint16_t crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len);
uint32_t crc32_ieee(const uint8_t *data, size_t len);
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len);
uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value);

bool util_memeq(const uint8_t *a, const uint8_t *b, size_t len);
bool util_eq(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen);
char *utf8_lcpy(char *dst, const char *src, size_t n);
int utf8_count_chars(const char *s);

static inline size_t sys_count_bits(const void *value, size_t len)
{
	size_t cnt = 0U;
	size_t i = 0U;

	for (; i < len; i++) {
		uint8_t value_u8 = ((const uint8_t *)value)[i];

		/* Implements Brian Kernighan’s Algorithm to count bits */
		while (value_u8) {
			value_u8 &= (value_u8 - 1);
			cnt++;
		}
	}

	return cnt;
}

static inline void mem_xor_n(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, size_t len)
{
	while (len--) {
		*dst++ = *src1++ ^ *src2++;
	}
}

static inline void mem_xor_128(uint8_t dst[16], const uint8_t src1[16], const uint8_t src2[16])
{
	mem_xor_n(dst, src1, src2, 16);
}

static inline bool u16_add_overflow(uint16_t a, uint16_t b, uint16_t *result)
{
	uint16_t c = a + b;

	*result = c;

	return c < a;
}

static inline unsigned int find_msb_set(uint32_t op)
{
	if (op == 0) {
		return 0;
	}

	unsigned int pos = 0;

	while (op != 0) {
		op >>= 1;
		pos++;
	}

	return pos;
}

static inline unsigned int find_lsb_set(uint32_t op)
{
	if (op == 0) {
		return 0;
	}

	unsigned int pos = 1;

	while ((op & 1) == 0) {
		op >>= 1;
		pos++;
	}

	return pos;
}

#endif /* __BASE_UTILS_INTERNAL_H__ */