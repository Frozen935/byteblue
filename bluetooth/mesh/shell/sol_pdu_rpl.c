/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/shell.h>

#include "utils.h"

static int cmd_srpl_clear(const struct bt_shell *sh, size_t argc,
			  char *argv[])
{
	int err = 0;
	uint8_t len;

	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_APP(bt_mesh_shell_target_ctx.app_idx,
							      bt_mesh_shell_target_ctx.dst);

	uint16_t range_start = bt_shell_strtoul(argv[1], 0, &err);
	bool acked = bt_shell_strtobool(argv[2], 0, &err);

	if (argc > 3) {
		len = bt_shell_strtoul(argv[3], 0, &err);
	} else {
		len = 0;
	}

	if (err < 0) {
		bt_shell_error("Invalid command parameter (err %d)", err);
		return err;
	}

	if (acked) {
		uint16_t start_rsp;
		uint8_t len_rsp;

		err = bt_mesh_sol_pdu_rpl_clear(&ctx, range_start, len,
						&start_rsp, &len_rsp);

		if (err) {
			bt_shell_error("Failed to clear Solicitation PDU RPL (err %d)", err);
		} else {
			bt_shell_print("Cleared Solicitation PDU RPL with range start=%u len=%u",
				    start_rsp, len_rsp);
		}

		return err;
	}

	err = bt_mesh_sol_pdu_rpl_clear_unack(&ctx, range_start, len);
	if (err) {
		bt_shell_error("Failed to clear Solicitation PDU RPL (err %d)", err);
	}

	return err;
}

BT_SHELL_SUBCMD_SET_CREATE(
	sol_pdu_rpl_cmds,
	BT_SHELL_CMD_ARG(clear, NULL, "<RngStart> <Ackd> [RngLen]",
		      cmd_srpl_clear, 3, 1),
	BT_SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), sol_pdu_rpl, &sol_pdu_rpl_cmds,
		 "Solicitation PDU RPL Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
