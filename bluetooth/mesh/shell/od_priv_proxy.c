/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/shell.h>

#include "utils.h"

static int cmd_od_priv_gatt_proxy_set(const struct bt_shell *sh, size_t argc,
				      char *argv[])
{
	uint8_t val, val_rsp;
	uint16_t net_idx = bt_mesh_shell_target_ctx.net_idx;
	uint16_t addr = bt_mesh_shell_target_ctx.dst;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_od_priv_proxy_cli_get(net_idx, addr, &val_rsp);
	} else {
		val = bt_shell_strtoul(argv[1], 0, &err);

		if (err) {
			bt_shell_warn("Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_od_priv_proxy_cli_set(net_idx, addr, val, &val_rsp);
	}

	if (err) {
		bt_shell_print("Unable to send On-Demand Private GATT Proxy Get/Set (err %d)",
			    err);
		return 0;
	}

	bt_shell_print("On-Demand Private GATT Proxy is set to 0x%02x", val_rsp);

	return 0;
}

BT_SHELL_SUBCMD_SET_CREATE(
	od_priv_proxy_cmds,
	BT_SHELL_CMD_ARG(gatt-proxy, NULL, "[Dur(s)]", cmd_od_priv_gatt_proxy_set, 1, 1),
	BT_SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), od_priv_proxy, &od_priv_proxy_cmds,
		 "On-Demand Private Proxy Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
