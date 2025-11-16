#ifndef __BASE_DEBUG_H__
#define __BASE_DEBUG_H__

#include <stdarg.h>
#include <stddef.h>

void bt_debug_vprint(const char *fmt, va_list args);
void bt_debug_print(const char *fmt, ...);
void bt_debug_hexdump(const char *prefix, const void *data, size_t len);

#endif