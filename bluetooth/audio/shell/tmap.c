/** @file
 *  @brief Bluetooth Telephone and Media Audio Profile shell
 *
 */

/*
 * Copyright (c) 2023-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <bluetooth/audio/tmap.h>
#include <bluetooth/conn.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static int cmd_tmap_init(const struct bt_shell *sh, size_t argc, char **argv)
{
	const enum bt_tmap_role role =
		(IS_ENABLED(CONFIG_BT_TMAP_CG_SUPPORTED) ? BT_TMAP_ROLE_CG : 0U) |
		(IS_ENABLED(CONFIG_BT_TMAP_CT_SUPPORTED) ? BT_TMAP_ROLE_CT : 0U) |
		(IS_ENABLED(CONFIG_BT_TMAP_UMS_SUPPORTED) ? BT_TMAP_ROLE_UMS : 0U) |
		(IS_ENABLED(CONFIG_BT_TMAP_UMR_SUPPORTED) ? BT_TMAP_ROLE_UMR : 0U) |
		(IS_ENABLED(CONFIG_BT_TMAP_BMS_SUPPORTED) ? BT_TMAP_ROLE_BMS : 0U) |
		(IS_ENABLED(CONFIG_BT_TMAP_BMR_SUPPORTED) ? BT_TMAP_ROLE_BMR : 0U);
	int err;

	bt_shell_info("Registering TMAS with role: 0x%04X", role);

	err = bt_tmap_register(role);
	if (err != 0) {
		bt_shell_error("bt_tmap_register (err %d)", err);

		return -ENOEXEC;
	}

	return 0;
}

static void tmap_discover_cb(enum bt_tmap_role role, struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("tmap discovery (err %d)", err);
		return;
	}

	bt_shell_print("tmap discovered for conn %p: role 0x%04x", conn, role);
}

static const struct bt_tmap_cb tmap_cb = {
	.discovery_complete = tmap_discover_cb,
};

static int cmd_tmap_discover(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");

		return -ENOEXEC;
	}

	err = bt_tmap_discover(default_conn, &tmap_cb);
	if (err != 0) {
		bt_shell_error("bt_tmap_discover (err %d)", err);

		return -ENOEXEC;
	}

	return err;
}

static int cmd_tmap(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		bt_shell_error("%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(tmap_cmds,
	BT_SHELL_CMD_ARG(init, NULL, "Initialize and register the TMAS", cmd_tmap_init, 1, 0),
	BT_SHELL_CMD_ARG(discover, NULL, "Discover TMAS on remote device", cmd_tmap_discover, 1, 0),
	BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(tmap, &tmap_cmds, "Bluetooth tmap client shell commands", cmd_tmap, 1, 1);

int bt_shell_cmd_tmap_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &tmap);
}
