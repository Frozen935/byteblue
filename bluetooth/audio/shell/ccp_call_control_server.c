/** @file
 *  @brief Bluetooth Call Control Profile Call Control Server shell
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include <bluetooth/audio/tbs.h>
#include <bluetooth/audio/ccp.h>

static struct bt_ccp_call_control_server_bearer
	*bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT];

static int cmd_ccp_call_control_server_init(const struct bt_shell *sh, size_t argc, char *argv[])
{
	static bool registered;

	if (registered) {
		bt_shell_info("Already initialized");

		return -ENOEXEC;
	}

	const struct bt_tbs_register_param gtbs_param = {
		.provider_name = "Generic TBS",
		.uci = "un000",
		.uri_schemes_supported = "tel,skype",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
	};
	int err;

	err = bt_ccp_call_control_server_register_bearer(&gtbs_param, &bearers[0]);
	if (err != 0) {
		bt_shell_error("Failed to register GTBS bearer: %d", err);

		return -ENOEXEC;
	}

	bt_shell_info("Registered GTBS bearer");

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		char prov_name[22]; /* Enough to store "Telephone Bearer #255" */
		const struct bt_tbs_register_param tbs_param = {
			.provider_name = prov_name,
			.uci = "un000",
			.uri_schemes_supported = "tel,skype",
			.gtbs = false,
			.authorization_required = false,
			/* Set different technologies per bearer */
			.technology = (i % BT_TBS_TECHNOLOGY_WCDMA) + 1,
			.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
		};

		snprintf(prov_name, sizeof(prov_name), "Telephone Bearer #%d", i);

		err = bt_ccp_call_control_server_register_bearer(&tbs_param, &bearers[i]);
		if (err != 0) {
			bt_shell_error("Failed to register bearer[%d]: %d", i, err);

			return -ENOEXEC;
		}

		bt_shell_info("Registered bearer[%d]", i);
	}

	registered = true;

	return 0;
}

static int validate_and_get_index(const struct bt_shell *sh, const char *index_arg)
{
	unsigned long index;
	int err = 0;

	index = bt_shell_strtoul(index_arg, 0, &err);
	if (err != 0) {
		bt_shell_error("Could not parse index: %d", err);

		return -ENOEXEC;
	}

	if (index >= CONFIG_BT_TBS_BEARER_COUNT) {
		bt_shell_error("Invalid index: %lu", index);

		return -ENOEXEC;
	}

	return (int)index;
}

static int cmd_ccp_call_control_server_set_bearer_name(const struct bt_shell *sh, size_t argc,
						       char *argv[])
{
	const char *name;
	int index = 0;
	int err = 0;

	if (argc > 2) {
		index = validate_and_get_index(sh, argv[1]);
		if (index < 0) {
			return index;
		}
	}

	name = argv[argc - 1];

	err = bt_ccp_call_control_server_set_bearer_provider_name(bearers[index], name);
	if (err != 0) {
		bt_shell_error("Failed to set bearer[%d] name: %d", index, err);

		return -ENOEXEC;
	}

	bt_shell_print("Bearer[%d] name: %s", index, name);

	return 0;
}

static int cmd_ccp_call_control_server_get_bearer_name(const struct bt_shell *sh, size_t argc,
						       char *argv[])
{
	const char *name;
	int index = 0;
	int err = 0;

	if (argc > 1) {
		index = validate_and_get_index(sh, argv[1]);
		if (index < 0) {
			return index;
		}
	}

	err = bt_ccp_call_control_server_get_bearer_provider_name(bearers[index], &name);
	if (err != 0) {
		bt_shell_error("Failed to get bearer[%d] name: %d", index, err);

		return -ENOEXEC;
	}

	bt_shell_print("Bearer[%d] name: %s", index, name);

	return 0;
}

static int cmd_ccp_call_control_server(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		bt_shell_error("%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(ccp_call_control_server_cmds,
			       BT_SHELL_CMD_ARG(init, NULL, "Initialize CCP Call Control Server",
					     cmd_ccp_call_control_server_init, 1, 0),
			       BT_SHELL_CMD_ARG(set_bearer_name, NULL,
					     "Set bearer name [index] <name>",
					     cmd_ccp_call_control_server_set_bearer_name, 2, 1),
			       BT_SHELL_CMD_ARG(get_bearer_name, NULL, "Get bearer name [index]",
					     cmd_ccp_call_control_server_get_bearer_name, 1, 1),
			       BT_SHELL_SUBCMD_SET_END);

BT_SHELL_CMD_ARG_DEFINE(ccp_call_control_server, &ccp_call_control_server_cmds,
		       "Bluetooth CCP Call Control Server shell commands",
		       cmd_ccp_call_control_server, 1, 1);

int bt_shell_cmd_ccp_call_control_server_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &ccp_call_control_server);
}
