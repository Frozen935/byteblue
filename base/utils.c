/* Misc shim helpers for build-only: hex2bin, crc16_reflect */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

static int hexval(int c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	return -1;
}

int char2hex(char c, uint8_t *x)
{
	if ((c >= '0') && (c <= '9')) {
		*x = c - '0';
	} else if ((c >= 'a') && (c <= 'f')) {
		*x = c - 'a' + 10;
	} else if ((c >= 'A') && (c <= 'F')) {
		*x = c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

int hex2char(uint8_t x, char *c)
{
	if (x <= 9) {
		*c = x + (char)'0';
	} else  if (x <= 15) {
		*c = x - 10 + (char)'a';
	} else {
		return -EINVAL;
	}

	return 0;
}

size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen)
{
	if (hexlen < ((buflen * 2U) + 1U)) {
		return 0;
	}

	for (size_t i = 0; i < buflen; i++) {
		hex2char(buf[i] >> 4, &hex[2U * i]);
		hex2char(buf[i] & 0xf, &hex[2U * i + 1U]);
	}

	hex[2U * buflen] = '\0';
	return 2U * buflen;
}

/* Convert hex string to bytes. Ignores spaces and common separators. */
size_t hex2bin(const char *hex, size_t hex_len, uint8_t *out, size_t out_size)
{
	if (!hex || !out || out_size == 0) {
		return 0;
	}
	size_t written = 0;
	int nibble = -1;
	for (size_t i = 0; i < hex_len && written < out_size; i++) {
		int v = hexval((unsigned char)hex[i]);
		if (v < 0) {
			/* Skip separators */
			continue;
		}
		if (nibble < 0) {
			nibble = v;
		} else {
			out[written++] = (uint8_t)((nibble << 4) | v);
			nibble = -1;
		}
	}
	return written;
}

/* Reflective CRC16 placeholder: returns 0 for build-only */
uint16_t crc16_reflect(uint16_t poly, uint16_t seed, const uint8_t *src, size_t len)
{
	uint16_t crc = seed;
	size_t i, j;

	for (i = 0; i < len; i++) {
		crc ^= (uint16_t)src[i];

		for (j = 0; j < 8; j++) {
			if (crc & 0x0001UL) {
				crc = (crc >> 1U) ^ poly;
			} else {
				crc = crc >> 1U;
			}
		}
	}

	return crc;
}

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
	return crc32_ieee_update(0x0, data, len);
}

uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len)
{
	/* crc table generated from polynomial 0xedb88320 */
	static const uint32_t table[16] = {
		0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU, 0x76dc4190U, 0x6b6b51f4U,
		0x4db26158U, 0x5005713cU, 0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
		0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU,
	};

	crc = ~crc;

	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];

		crc = (crc >> 4) ^ table[(crc ^ byte) & 0x0f];
		crc = (crc >> 4) ^ table[(crc ^ ((uint32_t)byte >> 4)) & 0x0f];
	}

	return (~crc);
}

uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value)
{
	uint8_t divisor = 100;
	uint8_t num_digits = 0;
	uint8_t digit;

	while ((buflen > 0) && (divisor > 0)) {
		digit = value / divisor;
		if ((digit != 0) || (divisor == 1) || (num_digits != 0)) {
			*buf = digit + (char)'0';
			buf++;
			buflen--;
			num_digits++;
		}

		value -= digit * divisor;
		divisor /= 10;
	}

	if (buflen != 0) {
		*buf = '\0';
	}

	return num_digits;
}

bool util_memeq(const uint8_t *a, const uint8_t *b, size_t len)
{
	return memcmp(a, b, len) == 0;
}

bool util_eq(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen)
{
	return alen == blen && (a == b || util_memeq(a, b, alen));
}

#define ASCII_CHAR          0x7F
#define SEQUENCE_FIRST_MASK 0xC0
#define SEQUENCE_LEN_2_BYTE 0xC0
#define SEQUENCE_LEN_3_BYTE 0xE0
#define SEQUENCE_LEN_4_BYTE 0xF0
#define MSB_SET             0x80

char *utf8_trunc(char *utf8_str)
{
	const size_t len = strlen(utf8_str);

	if (len == 0U) {
		/* no-op */
		return utf8_str;
	}

	char *last_byte_p = utf8_str + len - 1U;
	uint8_t bytes_truncated;
	char seq_start_byte;

	if ((*last_byte_p & ASCII_CHAR) == *last_byte_p) {
		/* Not part of an UTF8 sequence, return */
		return utf8_str;
	}

	/* Find the starting byte and NULL-terminate other bytes */
	bytes_truncated = 0;
	while ((*last_byte_p & SEQUENCE_FIRST_MASK) != SEQUENCE_FIRST_MASK &&
	       last_byte_p > utf8_str) {
		last_byte_p--;
		bytes_truncated++;
	}
	bytes_truncated++; /* include the starting byte */

	/* Verify if the last character actually need to be truncated
	 * Handles the case where the number of bytes in the last UTF8-char
	 * matches the number of bytes we searched for the starting byte
	 */
	seq_start_byte = *last_byte_p;
	if ((seq_start_byte & SEQUENCE_LEN_4_BYTE) == SEQUENCE_LEN_4_BYTE) {
		if (bytes_truncated == 4) {
			return utf8_str;
		}
	} else if ((seq_start_byte & SEQUENCE_LEN_3_BYTE) == SEQUENCE_LEN_3_BYTE) {
		if (bytes_truncated == 3) {
			return utf8_str;
		}
	} else if ((seq_start_byte & SEQUENCE_LEN_2_BYTE) == SEQUENCE_LEN_2_BYTE) {
		if (bytes_truncated == 2) {
			return utf8_str;
		}
	}

	/* NULL-terminate the unterminated starting byte */
	*last_byte_p = '\0';

	return utf8_str;
}

char *utf8_lcpy(char *dst, const char *src, size_t n)
{
	if (n > 0) {
		strncpy(dst, src, n - 1);
		dst[n - 1] = '\0';

		if (n != 1) {
			utf8_trunc(dst);
		}
	}

	return dst;
}

int utf8_count_chars(const char *s)
{
	int count = 0;
	const char *p = s; /* getting a pointer to increment */

	while (*p != '\0') {
		if ((*p & MSB_SET) == 0) { /* 1-byte character: 0xxxxxxx */
			p += 1;
		} else if ((*p & SEQUENCE_LEN_3_BYTE) == SEQUENCE_FIRST_MASK) {
			/* 2-byte character: 110xxxxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 2;
		} else if ((*p & SEQUENCE_LEN_4_BYTE) == SEQUENCE_LEN_3_BYTE) {
			/* 3-byte character: 1110xxxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET ||
			    (p[2] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 3;
		} else if ((*p & 0xF8) == SEQUENCE_LEN_4_BYTE) {
			/* 4-byte character: 11110xxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET ||
			    (p[2] & SEQUENCE_FIRST_MASK) != MSB_SET ||
			    (p[3] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 4;
		} else {
			/* Invalid UTF-8 byte (return) */
			return -EINVAL;
		}
		count++;
	}

	return count;
}
