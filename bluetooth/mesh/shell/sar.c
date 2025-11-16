/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/shell.h>

#include "utils.h"
#include "../sar_cfg_internal.h"

static int cmd_tx_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_sar_tx rsp;
	int err;

	err = bt_mesh_sar_cfg_cli_transmitter_get(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		bt_shell_error(
			    "Failed to send SAR Transmitter Get (err %d)", err);
		return 0;
	}

	bt_shell_print("Transmitter Get: %u %u %u %u %u %u %u",
		    rsp.seg_int_step, rsp.unicast_retrans_count,
		    rsp.unicast_retrans_without_prog_count,
		    rsp.unicast_retrans_int_step, rsp.unicast_retrans_int_inc,
		    rsp.multicast_retrans_count, rsp.multicast_retrans_int);

	return 0;
}

static int cmd_tx_set(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_sar_tx set, rsp;
	int err = 0;

	set.seg_int_step = bt_shell_strtoul(argv[1], 0, &err);
	set.unicast_retrans_count = bt_shell_strtoul(argv[2], 0, &err);
	set.unicast_retrans_without_prog_count = bt_shell_strtoul(argv[3], 0, &err);
	set.unicast_retrans_int_step = bt_shell_strtoul(argv[4], 0, &err);
	set.unicast_retrans_int_inc = bt_shell_strtoul(argv[5], 0, &err);
	set.multicast_retrans_count = bt_shell_strtoul(argv[6], 0, &err);
	set.multicast_retrans_int = bt_shell_strtoul(argv[7], 0, &err);

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_sar_cfg_cli_transmitter_set(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, &set, &rsp);
	if (err) {
		bt_shell_error("Failed to send SAR Transmitter Set (err %d)", err);
		return 0;
	}

	bt_shell_print("Transmitter Set: %u %u %u %u %u %u %u",
		    rsp.seg_int_step, rsp.unicast_retrans_count,
		    rsp.unicast_retrans_without_prog_count,
		    rsp.unicast_retrans_int_step, rsp.unicast_retrans_int_inc,
		    rsp.multicast_retrans_count, rsp.multicast_retrans_int);

	return 0;
}

static int cmd_rx_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_sar_rx rsp;
	int err;

	err = bt_mesh_sar_cfg_cli_receiver_get(bt_mesh_shell_target_ctx.net_idx,
					       bt_mesh_shell_target_ctx.dst, &rsp);
	if (err) {
		bt_shell_error("Failed to send SAR Receiver Get (err %d)", err);
		return 0;
	}

	bt_shell_print("Receiver Get: %u %u %u %u %u", rsp.seg_thresh,
		    rsp.ack_delay_inc, rsp.ack_retrans_count,
		    rsp.discard_timeout, rsp.rx_seg_int_step);

	return 0;
}

static int cmd_rx_set(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_sar_rx set, rsp;
	int err = 0;

	set.seg_thresh = bt_shell_strtoul(argv[1], 0, &err);
	set.ack_delay_inc = bt_shell_strtoul(argv[2], 0, &err);
	set.ack_retrans_count = bt_shell_strtoul(argv[3], 0, &err);
	set.discard_timeout = bt_shell_strtoul(argv[4], 0, &err);
	set.rx_seg_int_step = bt_shell_strtoul(argv[5], 0, &err);

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_sar_cfg_cli_receiver_set(bt_mesh_shell_target_ctx.net_idx,
					       bt_mesh_shell_target_ctx.dst, &set, &rsp);
	if (err) {
		bt_shell_error("Failed to send SAR Receiver Set (err %d)", err);
		return 0;
	}

	bt_shell_print("Receiver Set: %u %u %u %u %u", rsp.seg_thresh,
		    rsp.ack_delay_inc, rsp.ack_retrans_count,
		    rsp.discard_timeout, rsp.rx_seg_int_step);

	return 0;
}

BT_SHELL_SUBCMD_SET_CREATE(
	sar_cfg_cli_cmds, BT_SHELL_CMD_ARG(tx-get, NULL, NULL, cmd_tx_get, 1, 0),
	BT_SHELL_CMD_ARG(tx-set, NULL,
		      "<SegIntStep> <UniRetransCnt> <UniRetransWithoutProgCnt> "
		      "<UniRetransIntStep> <UniRetransIntInc> <MultiRetransCnt> "
		      "<MultiRetransInt>",
		      cmd_tx_set, 8, 0),
	BT_SHELL_CMD_ARG(rx-get, NULL, NULL, cmd_rx_get, 1, 0),
	BT_SHELL_CMD_ARG(rx-set, NULL,
		      "<SegThresh> <AckDelayInc> <DiscardTimeout> "
		      "<RxSegIntStep> <AckRetransCount>",
		      cmd_rx_set, 6, 0),
	BT_SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), sar, &sar_cfg_cli_cmds, "Sar Cfg Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
