/** @file
 *  @brief Bluetooth Telephone Bearer Service shell
 */

/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <bluetooth/addr.h>
#include <bluetooth/audio/tbs.h>
#include <bluetooth/conn.h>

#include "host/shell/bt.h"

static struct bt_conn *tbs_authorized_conn;

static bool tbs_authorize_cb(struct bt_conn *conn)
{
	return conn == tbs_authorized_conn;
}

static bool tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  const char *uri)
{
	/* Always accept calls */
	return true;
}

static struct bt_tbs_cb tbs_cbs = {
	.originate_call = tbs_originate_call_cb,
	.authorize = tbs_authorize_cb
};

static int cmd_tbs_authorize(const struct bt_shell *sh, size_t argc, char *argv[])
{
	char addr[BT_ADDR_LE_STR_LEN];

	tbs_authorized_conn = default_conn;

	(void)bt_addr_le_to_str(bt_conn_get_dst(tbs_authorized_conn),
				addr, sizeof(addr));

	bt_shell_print("Connection with addr %s authorized", addr);

	return 0;
}

static int cmd_tbs_init(const struct bt_shell *sh, size_t argc, char *argv[])
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

	err = bt_tbs_register_bearer(&gtbs_param);
	if (err < 0) {
		bt_shell_error("Failed to register GTBS: %d", err);

		return -ENOEXEC;
	}

	bt_shell_info("Registered GTBS");

	for (int i = 0; i < CONFIG_BT_TBS_BEARER_COUNT; i++) {
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

		err = bt_tbs_register_bearer(&tbs_param);
		if (err < 0) {
			bt_shell_error("Failed to register TBS[%d]: %d", i, err);

			return -ENOEXEC;
		}

		bt_shell_info("Registered TBS[%d] with index %u", i, (uint8_t)err);
	}

	bt_tbs_register_cb(&tbs_cbs);
	registered = true;

	return 0;
}

static int cmd_tbs_accept(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_accept((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_terminate(const struct bt_shell *sh, size_t argc,
			     char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_terminate((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_hold(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_hold((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_retrieve(const struct bt_shell *sh, size_t argc,
			    char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_retrieve((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_originate(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long service_index;
	uint8_t call_index;
	int result = 0;

	if (argc > 2) {
		service_index = bt_shell_strtoul(argv[1], 0, &result);
		if (result != 0) {
			bt_shell_error("Could not parse service_index: %d",
				    result);

			return -ENOEXEC;
		}

		if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
			bt_shell_error("Invalid service_index: %lu",
				    service_index);

			return -ENOEXEC;
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	result = bt_tbs_originate((uint8_t)service_index, argv[argc - 1],
				  &call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS call_index %u originated", call_index);
	}

	return result;
}

static int cmd_tbs_join(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint8_t call_indexes[CONFIG_BT_TBS_MAX_CALLS];
	unsigned long call_index;
	int result = 0;

	for (size_t i = 1; i < argc; i++) {
		call_index = bt_shell_strtoul(argv[i], 0, &result);
		if (result != 0) {
			bt_shell_error("Could not parse call_index: %d",
				    result);

			return -ENOEXEC;
		}

		if (call_index > UINT8_MAX) {
			bt_shell_error("Invalid call_index: %lu", call_index);

			return -ENOEXEC;
		}

		call_indexes[i - 1] = (uint8_t)call_index;
	}

	result = bt_tbs_join(argc - 1, call_indexes);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS join succeeded");
	}

	return result;
}

static int cmd_tbs_answer(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_remote_answer((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_remote_hold(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_remote_hold((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_remote_retrieve(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_remote_retrieve((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_remote_terminate(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	unsigned long call_index;
	int result = 0;

	call_index = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_remote_terminate((uint8_t)call_index);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded for call_index: %ld",
			    call_index);
	}

	return result;
}

static int cmd_tbs_incoming(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long service_index;
	int result = 0;

	if (argc > 4) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	result = bt_tbs_remote_incoming((uint8_t)service_index,
					argv[argc - 3],
					argv[argc - 2],
					argv[argc - 1]);
	if (result < 0) {
		bt_shell_print("TBS failed: %d", result);
	} else {
		bt_shell_print("TBS succeeded");
	}

	return result;
}

static int cmd_tbs_set_bearer_provider_name(const struct bt_shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long service_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	result = bt_tbs_set_bearer_provider_name((uint8_t)service_index,
						 argv[argc - 1]);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("Could not set provider name: %d", result);
	}

	return result;
}

static int cmd_tbs_set_bearer_technology(const struct bt_shell *sh, size_t argc,
					 char *argv[])
{
	unsigned long service_index;
	unsigned long technology;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	technology = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse technology: %d", result);

		return -ENOEXEC;
	}

	if (technology > UINT8_MAX) {
		bt_shell_error("Invalid technology: %lu", technology);

		return -ENOEXEC;
	}

	result = bt_tbs_set_bearer_technology((uint8_t)service_index,
					      (uint8_t)technology);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("Could not set technology: %d", result);
	}

	return result;
}

static int cmd_tbs_set_bearer_signal_strength(const struct bt_shell *sh,
					      size_t argc, char *argv[])
{
	unsigned long signal_strength;
	unsigned long service_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	signal_strength = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse signal_strength: %d", result);

		return -ENOEXEC;
	}

	if (signal_strength > UINT8_MAX) {
		bt_shell_error("Invalid signal_strength: %lu",
			    signal_strength);

		return -ENOEXEC;
	}

	result = bt_tbs_set_signal_strength((uint8_t)service_index,
					    (uint8_t)signal_strength);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("Could not set signal strength: %d", result);
	}

	return result;
}

static int cmd_tbs_set_status_flags(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	unsigned long service_index;
	unsigned long status_flags;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	status_flags = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse status_flags: %d", result);

		return -ENOEXEC;
	}

	if (status_flags > UINT8_MAX) {
		bt_shell_error("Invalid status_flags: %lu", status_flags);

		return -ENOEXEC;
	}

	result = bt_tbs_set_status_flags((uint8_t)service_index,
					 (uint16_t)status_flags);
	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("Could not set status flags: %d", result);
	}

	return result;
}

static int cmd_tbs_set_uri_scheme_list(const struct bt_shell *sh, size_t argc,
				       char *argv[])
{
	unsigned long service_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			service_index = BT_TBS_GTBS_INDEX;
		} else {
			service_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Could not parse service_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (service_index > CONFIG_BT_TBS_BEARER_COUNT) {
				bt_shell_error("Invalid service_index: %lu",
					    service_index);

				return -ENOEXEC;
			}
		}
	} else {
		service_index = BT_TBS_GTBS_INDEX;
	}

	result = bt_tbs_set_uri_scheme_list((uint8_t)service_index,
					    (const char **)&argv[2],
					    argc - 2);

	if (result != BT_TBS_RESULT_CODE_SUCCESS) {
		bt_shell_print("Could not set URI prefix list: %d", result);
	}

	return result;
}

static int cmd_tbs_print_calls(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	if (IS_ENABLED(CONFIG_BT_TBS_LOG_LEVEL_DBG)) {
		bt_tbs_dbg_print_calls();
		return 0;
	}

	return -ENOEXEC;
}

static int cmd_tbs(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s", argv[0],
			    argv[1]);
	} else {
		bt_shell_error("%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(tbs_cmds,
	BT_SHELL_CMD_ARG(init, NULL,
		      "Initialize TBS",
		      cmd_tbs_init, 1, 0),
	BT_SHELL_CMD_ARG(authorize, NULL,
		      "Authorize the current connection",
		      cmd_tbs_authorize, 1, 0),
	BT_SHELL_CMD_ARG(accept, NULL,
		      "Accept call <call_index>",
		      cmd_tbs_accept, 2, 0),
	BT_SHELL_CMD_ARG(terminate, NULL,
		      "Terminate call <call_index>",
		      cmd_tbs_terminate, 2, 0),
	BT_SHELL_CMD_ARG(hold, NULL,
		      "Hold call <call_index>",
		      cmd_tbs_hold, 2, 0),
	BT_SHELL_CMD_ARG(retrieve, NULL,
		      "Retrieve call <call_index>",
		      cmd_tbs_retrieve, 2, 0),
	BT_SHELL_CMD_ARG(originate, NULL,
		      "Originate call [<instance_index>] <uri>",
		      cmd_tbs_originate, 2, 1),
#if CONFIG_BT_TBS_MAX_CALLS > 1
	BT_SHELL_CMD_ARG(join, NULL,
		      "Join calls <id> <id> [<id> [<id> [...]]]",
		      cmd_tbs_join, 3, CONFIG_BT_TBS_MAX_CALLS - 2),
#endif /* CONFIG_BT_TBS_MAX_CALLS > 1 */
	BT_SHELL_CMD_ARG(incoming, NULL,
		      "Simulate incoming remote call "
		      "[<{instance_index, gtbs}>] <local_uri> <remote_uri> "
		      "<remote_friendly_name>",
		      cmd_tbs_incoming, 4, 1),
	BT_SHELL_CMD_ARG(remote_answer, NULL,
		      "Simulate remote answer outgoing call <call_index>",
		      cmd_tbs_answer, 2, 0),
	BT_SHELL_CMD_ARG(remote_retrieve, NULL,
		      "Simulate remote retrieve <call_index>",
		      cmd_tbs_remote_retrieve, 2, 0),
	BT_SHELL_CMD_ARG(remote_terminate, NULL,
		      "Simulate remote terminate <call_index>",
		      cmd_tbs_remote_terminate, 2, 0),
	BT_SHELL_CMD_ARG(remote_hold, NULL,
		      "Simulate remote hold <call_index>",
		      cmd_tbs_remote_hold, 2, 0),
	BT_SHELL_CMD_ARG(set_bearer_provider_name, NULL,
		      "Set the bearer provider name [<{instance_index, gtbs}>] "
		      "<name>",
		      cmd_tbs_set_bearer_provider_name, 2, 1),
	BT_SHELL_CMD_ARG(set_bearer_technology, NULL,
		      "Set the bearer technology [<{instance_index, gtbs}>] "
		      "<technology>",
		      cmd_tbs_set_bearer_technology, 2, 1),
	BT_SHELL_CMD_ARG(set_bearer_signal_strength, NULL,
		      "Set the bearer signal strength "
		      "[<{instance_index, gtbs}>] <strength>",
		      cmd_tbs_set_bearer_signal_strength, 2, 1),
	BT_SHELL_CMD_ARG(set_status_flags, NULL,
		      "Set the bearer feature and status value "
		      "[<{instance_index, gtbs}>] <feature_and_status>",
		      cmd_tbs_set_status_flags, 2, 1),
	BT_SHELL_CMD_ARG(set_uri_scheme, NULL,
		      "Set the URI prefix list <bearer_idx> "
		      "<uri1 [uri2 [uri3 [...]]]>",
		      cmd_tbs_set_uri_scheme_list, 3, 30),
	BT_SHELL_CMD_ARG(print_calls, NULL,
		      "Output all calls in the debug log",
		      cmd_tbs_print_calls, 1, 0),
	BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(tbs, &tbs_cmds, "Bluetooth TBS shell commands",
		       cmd_tbs, 1, 1);

int bt_shell_cmd_tbs_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &tbs);
}
