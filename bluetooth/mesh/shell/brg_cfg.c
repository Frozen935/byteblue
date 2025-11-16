/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/shell.h>

#include "mesh/foundation.h"
#include "utils.h"

static int cmd_subnet_bridge_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	enum bt_mesh_brg_cfg_state rsp;
	int err;

	err = bt_mesh_brg_cfg_cli_get(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		bt_shell_error("Failed to send Subnet Bridge Get (err %d)", err);
		return -ENOEXEC;
	}

	bt_shell_print("Subnet Bridge State: %s",
		    (rsp == BT_MESH_BRG_CFG_ENABLED) ? "Enabled" : "Disabled");
	return 0;
}

static int cmd_subnet_bridge_set(const struct bt_shell *sh, size_t argc, char *argv[])
{
	enum bt_mesh_brg_cfg_state set, rsp;
	int err = 0;

	set = bt_shell_strtobool(argv[1], 0, &err) ? BT_MESH_BRG_CFG_ENABLED
						: BT_MESH_BRG_CFG_DISABLED;

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_set(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, set, &rsp);
	if (err) {
		bt_shell_error("Failed to send Subnet Bridge Set (err %d)", err);
		return -ENOEXEC;
	}

	bt_shell_print("Subnet Bridge State: %s",
		    (rsp == BT_MESH_BRG_CFG_ENABLED) ? "Enabled" : "Disabled");
	return 0;
}

static int cmd_bridging_table_size_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint16_t rsp;
	int err;

	err = bt_mesh_brg_cfg_cli_table_size_get(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		bt_shell_error("Failed to send Bridging Table Size Get (err %d)", err);
		return -ENOEXEC;
	}

	bt_shell_print("Bridging Table Size: %u", rsp);
	return 0;
}

static int cmd_bridging_table_add(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_brg_cfg_table_entry entry;
	struct bt_mesh_brg_cfg_table_status rsp;
	int err = 0;

	entry.directions = bt_shell_strtoul(argv[1], 0, &err);
	entry.net_idx1 = bt_shell_strtoul(argv[2], 0, &err);
	entry.net_idx2 = bt_shell_strtoul(argv[3], 0, &err);
	entry.addr1 = bt_shell_strtoul(argv[4], 0, &err);
	entry.addr2 = bt_shell_strtoul(argv[5], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_add(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, &entry, &rsp);
	if (err) {
		bt_shell_error("Failed to send Bridging Table Add (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		bt_shell_print("Bridging Table Add failed with status 0x%02x", rsp.status);
	} else {
		bt_shell_print("Bridging Table Add was successful.");
	}
	return 0;
}

static int cmd_bridging_table_remove(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx1, net_idx2, addr1, addr2;
	struct bt_mesh_brg_cfg_table_status rsp;
	int err = 0;

	net_idx1 = bt_shell_strtoul(argv[1], 0, &err);
	net_idx2 = bt_shell_strtoul(argv[2], 0, &err);
	addr1 = bt_shell_strtoul(argv[3], 0, &err);
	addr2 = bt_shell_strtoul(argv[4], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_remove(bt_mesh_shell_target_ctx.net_idx,
					       bt_mesh_shell_target_ctx.dst, net_idx1, net_idx2,
					       addr1, addr2, &rsp);
	if (err) {
		bt_shell_error("Failed to send Bridging Table Remove (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		bt_shell_print("Bridging Table Remove failed with status 0x%02x", rsp.status);
	} else {
		bt_shell_print("Bridging Table Remove was successful.");
	}
	return 0;
}

static int cmd_bridged_subnets_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_brg_cfg_filter_netkey filter_net_idx;
	uint8_t start_idx;
	struct bt_mesh_brg_cfg_subnets_list rsp = {
		.list = BT_BUF_SIMPLE(CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX * 3),
	};
	int err = 0;

	bt_buf_simple_init(rsp.list, 0);

	filter_net_idx.filter = bt_shell_strtoul(argv[1], 0, &err);
	filter_net_idx.net_idx = bt_shell_strtoul(argv[2], 0, &err);
	start_idx = bt_shell_strtoul(argv[3], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_subnets_get(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, filter_net_idx,
					      start_idx, &rsp);
	if (err) {
		bt_shell_error("Failed to send Bridged Subnets Get (err %d)", err);
		return -ENOEXEC;
	}

	bt_shell_print("Bridged Subnets List:");
	bt_shell_print("\tfilter: %02x", rsp.net_idx_filter.filter);
	bt_shell_print("\tnet_idx: %04x", rsp.net_idx_filter.net_idx);
	bt_shell_print("\tstart_idx: %u", rsp.start_idx);
	if (rsp.list) {
		uint16_t net_idx1, net_idx2;
		int i = 0;

		while (rsp.list->len) {
			key_idx_unpack_pair(rsp.list, &net_idx1, &net_idx2);
			bt_shell_print("\tEntry %d:", i++);
			bt_shell_print("\t\tnet_idx1: 0x%04x, net_idx2: 0x%04x", net_idx1,
				    net_idx2);
		}
	}
	return 0;
}

static int cmd_bridging_table_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx1, net_idx2, start_idx;
	struct bt_mesh_brg_cfg_table_list rsp = {
		.list = BT_BUF_SIMPLE(CONFIG_BT_MESH_BRG_TABLE_ITEMS_MAX * 5),
	};
	int err = 0;

	bt_buf_simple_init(rsp.list, 0);

	net_idx1 = bt_shell_strtoul(argv[1], 0, &err);
	net_idx2 = bt_shell_strtoul(argv[2], 0, &err);
	start_idx = bt_shell_strtoul(argv[3], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_brg_cfg_cli_table_get(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, net_idx1, net_idx2,
					    start_idx, &rsp);
	if (err) {
		bt_shell_error("Failed to send Bridging Table Get (err %d)", err);
		return -ENOEXEC;
	}

	if (rsp.status) {
		bt_shell_print("Bridging Table Get failed with status 0x%02x", rsp.status);
	} else {
		bt_shell_print("Bridging Table List:");
		bt_shell_print("\tstatus: %02x", rsp.status);
		bt_shell_print("\tnet_idx1: %04x", rsp.net_idx1);
		bt_shell_print("\tnet_idx2: %04x", rsp.net_idx2);
		bt_shell_print("\tstart_idx: %u", rsp.start_idx);
		if (rsp.list) {
			uint16_t addr1, addr2;
			uint8_t directions;
			int i = 0;

			while (rsp.list->len) {
				addr1 = bt_buf_simple_pull_le16(rsp.list);
				addr2 = bt_buf_simple_pull_le16(rsp.list);
				directions = bt_buf_simple_pull_u8(rsp.list);
				bt_shell_print("\tEntry %d:", i++);
				bt_shell_print(
					    "\t\taddr1: 0x%04x, addr2: 0x%04x, directions: 0x%02x",
					    addr1, addr2, directions);
			}
		}
	}
	return 0;
}

BT_SHELL_SUBCMD_SET_CREATE(
	brg_cfg_cmds, BT_SHELL_CMD_ARG(get, NULL, NULL, cmd_subnet_bridge_get, 1, 0),
	BT_SHELL_CMD_ARG(set, NULL, "<State(disable, enable)>", cmd_subnet_bridge_set, 2, 0),
	BT_SHELL_CMD_ARG(table-size-get, NULL, NULL, cmd_bridging_table_size_get, 1, 0),
	BT_SHELL_CMD_ARG(table-add, NULL, "<Directions> <NetIdx1> <NetIdx2> <Addr1> <Addr2>",
		      cmd_bridging_table_add, 6, 0),
	BT_SHELL_CMD_ARG(table-remove, NULL, "<NetIdx1> <NetIdx2> <Addr1> <Addr2>",
		      cmd_bridging_table_remove, 5, 0),
	BT_SHELL_CMD_ARG(subnets-get, NULL, "<Filter> <NetIdx> <StartIdx>", cmd_bridged_subnets_get,
		      4, 0),
	BT_SHELL_CMD_ARG(table-get, NULL, "<NetIdx1> <NetIdx2> <StartIdx>", cmd_bridging_table_get,
		      4, 0),
	BT_SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), brg, &brg_cfg_cmds, "Bridge Configuration Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
