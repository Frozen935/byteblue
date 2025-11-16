/** @file
 *  @brief Bluetooth Telephone Bearer Service client shell
 *
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/tbs.h>

#include "host/shell/bt.h"

static int cmd_tbs_client_discover(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	int result = 0;

	result = bt_tbs_client_discover(default_conn);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
static int cmd_tbs_client_set_signal_strength_interval(const struct bt_shell *sh,
						       size_t argc,
						       char *argv[])
{
	unsigned long inst_index;
	unsigned long interval;
	int result = 0;

	/* TODO: The handling of index/GTBS index is done in almost every
	 * function - Should add a helper function to reduce duplicated code
	 */
	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	interval = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Failed to parse interval: %d", result);

		return -ENOEXEC;
	}

	if (interval > UINT8_MAX) {
		bt_shell_error("Invalid interval: %lu", interval);

		return -ENOEXEC;
	}

	result = bt_tbs_client_set_signal_strength_interval(default_conn,
							    inst_index,
							    interval);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL)
static int cmd_tbs_client_hold(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	unsigned long inst_index;
	unsigned long call_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Failed to parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_client_hold_call(default_conn, inst_index, call_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL)
static int cmd_tbs_client_retrieve(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	unsigned long inst_index;
	unsigned long call_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Failed to parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_client_retrieve_call(default_conn, inst_index,
					     call_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL)
static int cmd_tbs_client_accept(const struct bt_shell *sh, size_t argc,
				 char *argv[])
{
	unsigned long inst_index;
	unsigned long call_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Failed to parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_client_accept_call(default_conn, inst_index,
					   call_index);

	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS)
static int cmd_tbs_client_join(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{

	uint8_t call_indexes[CONFIG_BT_TBS_CLIENT_MAX_CALLS];
	unsigned long inst_index;
	int result = 0;

	if (strcmp(argv[1], "gtbs") == 0) {
		inst_index = BT_TBS_GTBS_INDEX;
	} else {
		inst_index = bt_shell_strtoul(argv[1], 0, &result);
		if (result != 0) {
			bt_shell_error(
					"Failed to parse inst_index: %d",
					result);

			return -ENOEXEC;
		}

		if (inst_index > UINT8_MAX) {
			bt_shell_error("Invalid index: %lu",
					inst_index);

			return -ENOEXEC;
		}
	}

	for (size_t i = 2U; i < argc; i++) {
		unsigned long call_index;

		call_index = bt_shell_strtoul(argv[i], 0, &result);
		if (result != 0) {
			bt_shell_error("Failed to parse call_index: %d",
				    result);

			return -ENOEXEC;
		}

		if (call_index > UINT8_MAX) {
			bt_shell_error("Invalid call_index: %lu", call_index);

			return -ENOEXEC;
		}

		call_indexes[i - 2] = (uint8_t)call_index;
	}

	result = bt_tbs_client_join_calls(default_conn, inst_index,
					  call_indexes, argc - 2);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL)
static int cmd_tbs_client_terminate(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	unsigned long inst_index;
	unsigned long call_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = bt_shell_strtoul(argv[argc - 1], 0, &result);
	if (result != 0) {
		bt_shell_error("Failed to parse call_index: %d", result);

		return -ENOEXEC;
	}

	if (call_index > UINT8_MAX) {
		bt_shell_error("Invalid call_index: %lu", call_index);

		return -ENOEXEC;
	}

	result = bt_tbs_client_terminate_call(default_conn, inst_index,
					      call_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
static int cmd_tbs_client_originate(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 2) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_originate_call(default_conn, inst_index,
					      argv[argc - 1]);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
static int cmd_tbs_client_read_bearer_provider_name(const struct bt_shell *sh,
						    size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_bearer_provider_name(default_conn,
							 inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
static int cmd_tbs_client_read_bearer_uci(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_bearer_uci(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
static int cmd_tbs_client_read_technology(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_technology(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
static int cmd_tbs_client_read_uri_list(const struct bt_shell *sh, size_t argc,
					char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_uri_list(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
static int cmd_tbs_client_read_signal_strength(const struct bt_shell *sh,
					       size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_signal_strength(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */

#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
static int cmd_tbs_client_read_signal_interval(const struct bt_shell *sh,
					       size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_signal_interval(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */

#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
static int cmd_tbs_client_read_current_calls(const struct bt_shell *sh,
					     size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_current_calls(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */

#if defined(CONFIG_BT_TBS_CLIENT_CCID)
static int cmd_tbs_client_read_ccid(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_ccid(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
static int cmd_tbs_client_read_uri(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_call_uri(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */

#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
static int cmd_tbs_client_read_status_flags(const struct bt_shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_status_flags(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */

static int cmd_tbs_client_read_call_state(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_call_state(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
static int cmd_tbs_client_read_remote_uri(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_remote_uri(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
static int cmd_tbs_client_read_friendly_name(const struct bt_shell *sh,
					     size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_friendly_name(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */

#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
static int cmd_tbs_client_read_optional_opcodes(const struct bt_shell *sh,
						size_t argc, char *argv[])
{
	unsigned long inst_index;
	int result = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "gtbs") == 0) {
			inst_index = BT_TBS_GTBS_INDEX;
		} else {
			inst_index = bt_shell_strtoul(argv[1], 0, &result);
			if (result != 0) {
				bt_shell_error(
					    "Failed to parse inst_index: %d",
					    result);

				return -ENOEXEC;
			}

			if (inst_index > UINT8_MAX) {
				bt_shell_error("Invalid index: %lu",
					    inst_index);

				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_tbs_client_read_optional_opcodes(default_conn, inst_index);
	if (result != 0) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */

static int cmd_tbs_client(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		bt_shell_error("%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		bt_shell_error("%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(tbs_client_cmds,
	BT_SHELL_CMD_ARG(discover, NULL,
		      "Discover TBS",
		      cmd_tbs_client_discover, 1, 0),

#if defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL)
	BT_SHELL_CMD_ARG(set_signal_reporting_interval, NULL,
		      "Set the signal reporting interval "
		      "[<{instance_index, gtbs}>] <interval>",
		      cmd_tbs_client_set_signal_strength_interval, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_SET_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL)
	BT_SHELL_CMD_ARG(originate, NULL,
		      "Originate a call [<{instance_index, gtbs}>] <uri>",
		      cmd_tbs_client_originate, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_ORIGINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL)
	BT_SHELL_CMD_ARG(terminate, NULL,
		      "terminate a call [<{instance_index, gtbs}>] <id>",
		      cmd_tbs_client_terminate, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_TERMINATE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL)
	BT_SHELL_CMD_ARG(accept, NULL,
		      "Accept a call [<{instance_index, gtbs}>] <id>",
		      cmd_tbs_client_accept, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_ACCEPT_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL)
	BT_SHELL_CMD_ARG(hold, NULL,
		      "Place a call on hold [<{instance_index, gtbs}>] <id>",
		      cmd_tbs_client_hold, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_HOLD_CALL) */

#if defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL)
	BT_SHELL_CMD_ARG(retrieve, NULL,
		      "Retrieve a held call [<{instance_index, gtbs}>] <id>",
		      cmd_tbs_client_retrieve, 2, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_RETRIEVE_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS)
	BT_SHELL_CMD_ARG(join, NULL,
		      "Join calls <{instance_index, gtbs}> "
		      "<id> <id> [<id> [<id> [...]]]",
		      cmd_tbs_client_join, 4,
		      CONFIG_BT_TBS_CLIENT_MAX_CALLS - 2),
#endif /* defined(CONFIG_BT_TBS_CLIENT_JOIN_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME)
	BT_SHELL_CMD_ARG(read_provider_name, NULL,
		      "Read the bearer name [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_bearer_provider_name, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_PROVIDER_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI)
	BT_SHELL_CMD_ARG(read_bearer_uci, NULL,
		      "Read the bearer UCI [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_bearer_uci, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_UCI) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY)
	BT_SHELL_CMD_ARG(read_technology, NULL,
		      "Read the bearer technology [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_technology, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_TECHNOLOGY) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST)
	BT_SHELL_CMD_ARG(read_uri_list, NULL,
		      "Read the bearer's supported URI list "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_uri_list, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_URI_SCHEMES_SUPPORTED_LIST) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH)
	BT_SHELL_CMD_ARG(read_signal_strength, NULL,
		      "Read the bearer signal strength "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_signal_strength, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_SIGNAL_STRENGTH) */
#if defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL)
	BT_SHELL_CMD_ARG(read_signal_interval, NULL,
		      "Read the bearer signal strength reporting interval "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_signal_interval, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_READ_BEARER_SIGNAL_INTERVAL) */
#if defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS)
	BT_SHELL_CMD_ARG(read_current_calls, NULL,
		      "Read the current calls [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_current_calls, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_BEARER_LIST_CURRENT_CALLS) */
#if defined(CONFIG_BT_TBS_CLIENT_CCID)
	BT_SHELL_CMD_ARG(read_ccid, NULL,
		      "Read the CCID [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_ccid, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_CCID) */
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI)
	BT_SHELL_CMD_ARG(read_uri, NULL,
		      "Read the incoming call target URI "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_uri, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_URI) */
#if defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS)
	BT_SHELL_CMD_ARG(read_status_flags, NULL,
		      "Read the in feature and status value "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_status_flags, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_STATUS_FLAGS) */
	BT_SHELL_CMD_ARG(read_call_state, NULL,
		      "Read the call state [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_call_state, 1, 1),
#if defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL)
	BT_SHELL_CMD_ARG(read_remote_uri, NULL,
		      "Read the incoming remote URI [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_remote_uri, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_INCOMING_CALL) */
#if defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME)
	BT_SHELL_CMD_ARG(read_friendly_name, NULL,
		      "Read the friendly name of an incoming call "
		      "[<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_friendly_name, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_CALL_FRIENDLY_NAME) */
#if defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES)
	BT_SHELL_CMD_ARG(read_optional_opcodes, NULL,
		      "Read the optional opcodes [<{instance_index, gtbs}>]",
		      cmd_tbs_client_read_optional_opcodes, 1, 1),
#endif /* defined(CONFIG_BT_TBS_CLIENT_OPTIONAL_OPCODES) */
	BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(tbs_client, &tbs_client_cmds,
		       "Bluetooth TBS_CLIENT shell commands",
		       cmd_tbs_client, 1, 1);

int bt_shell_cmd_tbs_client_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &tbs_client);
}
