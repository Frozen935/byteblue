#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(__NuttX__)
#include <syslog.h>
#endif

#include "base/bt_debug.h"

void bt_debug_vprint(const char *fmt, va_list args)
{
#if defined(__linux__)
	vprintf(fmt, args);
#elif defined(__NuttX__)
	vsyslog(LOG_INFO, fmt, args);
    #else
    vprintf(fmt, args);
#endif
}

void bt_debug_print(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void bt_debug_hexdump(const char *prefix, const void *data, size_t len)
{
	const unsigned char *p = data;
	char hex[16 * 3 + 1];
	char ascii[16 + 1];
	int i, hex_len = 0, ascii_len = 0;

	if (!prefix) {
		prefix = "hexdump";
	}

	bt_debug_print("%s\n", prefix);
	for (i = 0; i < len; i++) {
		int mod = i % 16;

		if (mod == 0) {
			hex_len = 0;
			ascii_len = 0;
		}

		hex_len += snprintf(hex + hex_len, sizeof(hex) - hex_len, "%02x ", p[i]);
		ascii[ascii_len++] = isprint(p[i]) ? p[i] : '.';

		if (mod == 15 || i == len - 1) {
			hex[hex_len] = '\0';
			ascii[ascii_len] = '\0';

		bt_debug_print("%-48s %s\n", hex, ascii);
		}
	}
}