#ifndef ZEPHYR_SHIM_SETTINGS_H
#define ZEPHYR_SHIM_SETTINGS_H

#include <stddef.h>
#include <stdint.h>

/* Settings stubs */
int settings_name_steq(const char *name, const char *key, const char **next);
typedef int (*settings_read_cb)(void *cb_arg, const void *data, size_t len);
int settings_load_subtree_direct(const char *subtree,
				 int (*cb)(const char *key, size_t len, settings_read_cb read_cb,
					   void *cb_arg, void *param),
				 void *param);

/* Settings API lightweight placeholders */
/* Lightweight settings prototypes (compile-time only) */

static inline int settings_name_next(const char *name, const char **next)
{
	UNUSED(name);
	if (next) {
		*next = NULL;
	}
	/* Compile-only placeholder, no parsing semantics. */
	return 0;
}

static inline int settings_subsys_init(void)
{
	return 0;
}

static inline int settings_save_one(const char *key, const void *value, size_t len)
{
	UNUSED(key);
	UNUSED(value);
	UNUSED(len);
	return 0;
}

static inline int settings_load(void)
{
	return 0;
}

static inline int settings_delete(const char *key)
{
	UNUSED(key);
	return 0;
}

#ifndef SETTINGS_STATIC_HANDLER_DEFINE
#define SETTINGS_STATIC_HANDLER_DEFINE(name, subtree, get, set, commit, export) /* no-op */
#endif
#ifndef SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO
#define SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(name, subtree, get, set, commit, export,         \
						  cprio) /* no-op */
#endif
/* BT_SETTINGS_CPRIO_* are provided by bluetooth/common/bt_settings_commit.h in this tree */

#endif /* ZEPHYR_SHIM_SETTINGS_H */