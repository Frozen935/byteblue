#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "bt_storage.h"

int bt_storage_name_steq(const char *name, const char *key, const char **next)
{
	return -ENOTSUP;
}

int bt_storage_load_subtree_direct(const char *subtree, bt_storage_read_cb read_cb, void *param)
{
	return -ENOTSUP;
}

int bt_storage_name_next(const char *name, const char **next)
{
	UNUSED(name);
	if (next) {
		*next = NULL;
	}

	return -ENOTSUP;
}

int bt_storage_init(void)
{
	return -ENOTSUP;
}

int bt_storage_save_one(const char *key, const void *value, size_t len)
{
	UNUSED(key);
	UNUSED(value);
	UNUSED(len);
	return -ENOTSUP;
}

int bt_storage_load(void)
{
	return -ENOTSUP;
}

int bt_storage_delete(const char *key)
{
	UNUSED(key);
	return -ENOTSUP;
}