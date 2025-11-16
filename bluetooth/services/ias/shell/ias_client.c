/**
 * @file
 * @brief Shell APIs for Bluetooth IAS
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/services/ias.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static void ias_discover_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Failed to discover IAS err: %d\n", err);
	} else {
		bt_shell_print("IAS discover success\n");
	}
}

static struct bt_ias_client_cb ias_client_callbacks = {
	.discover = ias_discover_cb,
};

static int cmd_ias_client_init(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_ias_client_cb_register(&ias_client_callbacks);
	if (err != 0) {
		bt_shell_print("IAS client init failed");
	} else {
		bt_shell_print("IAS client initialized");
	}

	return err;
}

static int cmd_ias_client_discover(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_ias_discover(default_conn);
	if (err != 0) {
		bt_shell_print("IAS discover failed");
	}

	return err;
}

static int cmd_ias_client_set_alert(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err = 0;

	if (strcmp(argv[1], "stop") == 0) {
		err = bt_ias_client_alert_write(default_conn,
						BT_IAS_ALERT_LVL_NO_ALERT);
	} else if (strcmp(argv[1], "mild") == 0) {
		err = bt_ias_client_alert_write(default_conn,
						BT_IAS_ALERT_LVL_MILD_ALERT);
	} else if (strcmp(argv[1], "high") == 0) {
		err = bt_ias_client_alert_write(default_conn,
						BT_IAS_ALERT_LVL_HIGH_ALERT);
	} else {
		bt_shell_error("Invalid alert level %s", argv[1]);
		return -EINVAL;
	}

	if (err != 0) {
		bt_shell_error("Failed to send %s alert %d", argv[1], err);
	} else {
		bt_shell_print("Sent alert %s", argv[1]);
	}

	return err;
}

static int cmd_ias_client(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		bt_shell_error("%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(ias_cli_cmds,
	BT_SHELL_CMD_ARG(init, NULL,
		      "Initialize the client and register callbacks",
		      cmd_ias_client_init, 1, 0),
	BT_SHELL_CMD_ARG(discover, NULL,
		      "Discover IAS",
		      cmd_ias_client_discover, 1, 0),
	BT_SHELL_CMD_ARG(set_alert, NULL,
		      "Send alert <stop/mild/high>",
		      cmd_ias_client_set_alert, 2, 0),
		      BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(ias_client, &ias_cli_cmds, "Bluetooth IAS client shell commands",
		       cmd_ias_client, 1, 1);

int bt_shell_cmd_ias_client_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &ias_client);
}
