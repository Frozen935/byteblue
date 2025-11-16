/**
 * @file
 * @brief Bluetooth Hearing Access Service (HAS) shell.
 *
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <bluetooth/conn.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/has.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static struct bt_has *inst;

static void has_client_discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
				   enum bt_has_hearing_aid_type type,
				   enum bt_has_capabilities caps)
{
	if (err) {
		bt_shell_error("HAS discovery (err %d)", err);
		return;
	}

	bt_shell_print("HAS discovered %p type 0x%02x caps 0x%02x for conn %p",
		       has, type, caps, conn);

	inst = has;
}

static void has_client_preset_switch_cb(struct bt_has *has, int err, uint8_t index)
{
	if (err != 0) {
		bt_shell_error("HAS %p preset switch error (err %d)", has, err);
	} else {
		bt_shell_print("HAS %p preset switch index 0x%02x", has, index);
	}
}

static void has_client_preset_read_rsp_cb(struct bt_has *has, int err,
					  const struct bt_has_preset_record *record, bool is_last)
{
	if (err) {
		bt_shell_error("Preset Read operation failed (err %d)", err);
		return;
	}

	bt_shell_print("Preset Index: 0x%02x\tProperties: 0x%02x\tName: %s",
		       record->index, record->properties, record->name);

	if (is_last) {
		bt_shell_print("Preset Read operation complete");
	}
}

static const struct bt_has_client_cb has_client_cb = {
	.discover = has_client_discover_cb,
	.preset_switch = has_client_preset_switch_cb,
	.preset_read_rsp = has_client_preset_read_rsp_cb,
};

static int cmd_has_client_init(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;

	err = bt_has_client_cb_register(&has_client_cb);
	if (err != 0) {
		bt_shell_error("bt_has_client_cb_register (err %d)", err);
	}

	return err;
}

static int cmd_has_client_discover(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	err = bt_has_client_discover(default_conn);
	if (err != 0) {
		bt_shell_error("bt_has_client_discover (err %d)", err);
	}

	return err;
}

static int cmd_has_client_read_presets(const struct bt_shell *sh, size_t argc, char **argv)
{
	int err;
	const uint8_t index = bt_shell_strtoul(argv[1], 16, &err);
	const uint8_t count = bt_shell_strtoul(argv[2], 10, &err);

	if (err < 0) {
		bt_shell_error("Invalid command parameter (err %d)", err);
		return err;
	}

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		bt_shell_error("No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_presets_read(inst, index, count);
	if (err != 0) {
		bt_shell_error("bt_has_client_discover (err %d)", err);
	}

	return err;
}

static int cmd_has_client_preset_set(const struct bt_shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	uint8_t index;
	int err = 0;

	index = bt_shell_strtoul(argv[1], 16, &err);
	if (err < 0) {
		bt_shell_error("Invalid command parameter (err %d)", err);
		return -ENOEXEC;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			bt_shell_error("Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		bt_shell_error("No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_set(inst, index, sync);
	if (err != 0) {
		bt_shell_error("bt_has_client_preset_switch (err %d)", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_has_client_preset_next(const struct bt_shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	int err;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			bt_shell_error("Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		bt_shell_error("No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_next(inst, sync);
	if (err != 0) {
		bt_shell_error("bt_has_client_preset_next (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_has_client_preset_prev(const struct bt_shell *sh, size_t argc, char **argv)
{
	bool sync = false;
	int err;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!strcmp(arg, "sync")) {
			sync = true;
		} else {
			bt_shell_error("Invalid argument");
			return -ENOEXEC;
		}
	}

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (!inst) {
		bt_shell_error("No instance discovered");
		return -ENOEXEC;
	}

	err = bt_has_client_preset_prev(inst, sync);
	if (err != 0) {
		bt_shell_error("bt_has_client_preset_prev (err %d)", err);
		return -ENOEXEC;
	}

	return err;
}

static int cmd_has_client(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		bt_shell_error("%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

BT_SHELL_SUBCMD_SET_CREATE(has_client_cmds,
	BT_SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_has_client_init, 1, 0),
	BT_SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_has_client_discover, 1, 0),
	BT_SHELL_CMD_ARG(presets_read, NULL, "<start_index_hex> <max_count_dec>",
		      cmd_has_client_read_presets, 3, 0),
	BT_SHELL_CMD_ARG(preset_set, NULL, "<index_hex> [sync]", cmd_has_client_preset_set, 2, 1),
	BT_SHELL_CMD_ARG(preset_next, NULL, "[sync]", cmd_has_client_preset_next, 1, 1),
	BT_SHELL_CMD_ARG(preset_prev, NULL, "[sync]", cmd_has_client_preset_prev, 1, 1),
	BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(has_client, &has_client_cmds, "Bluetooth HAS client shell commands",
		       cmd_has_client, 1, 1);

int bt_shell_cmd_has_client_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &has_client);
}
