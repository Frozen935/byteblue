/**
 * @file bt_shell_private.c
 * @brief Bluetooth shell private module
 *
 * Provide common function which can be shared using privately inside Bluetooth shell.
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <base/bt_debug.h>
#include <utils/bt_utils.h>
#include "bt_shell_private.h"

/**
 * @brief List of registered shell commands
 */

long bt_shell_strtol(const char *str, int base, int *err)
{
	long val;
	char *endptr;

	errno = 0;
	val = strtol(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

unsigned long bt_shell_strtoul(const char *str, int base, int *err)
{
	unsigned long val;
	char *endptr;

	if (*str == '-') {
		*err = -EINVAL;
		return 0;
	}

	errno = 0;
	val = strtoul(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

unsigned long long bt_shell_strtoull(const char *str, int base, int *err)
{
	unsigned long long val;
	char *endptr;

	if (*str == '-') {
		*err = -EINVAL;
		return 0;
	}

	errno = 0;
	val = strtoull(str, &endptr, base);
	if (errno == ERANGE) {
		*err = -ERANGE;
		return 0;
	} else if (errno || endptr == str || *endptr) {
		*err = -EINVAL;
		return 0;
	}

	return val;
}

bool bt_shell_strtobool(const char *str, int base, int *err)
{
	if (!strcmp(str, "on") || !strcmp(str, "enable") || !strcmp(str, "true")) {
		return true;
	}

	if (!strcmp(str, "off") || !strcmp(str, "disable") || !strcmp(str, "false")) {
		return false;
	}

	return bt_shell_strtoul(str, base, err);
}

static void bt_shell_vprintf(const char *level, const char *fmt, va_list args)
{
	(void)level;

	bt_debug_vprint(fmt, args);
}

void bt_shell_hexdump(const uint8_t *data, size_t len)
{
	bt_debug_hexdump("Shell hexdump:", data, len);
}

void bt_shell_fprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_shell_vprintf(BT_SHELL_PRINT, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_info(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_shell_vprintf(BT_SHELL_INFO, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_print(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_shell_vprintf(BT_SHELL_NORMAL, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_warn(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_shell_vprintf(BT_SHELL_WARN, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_shell_vprintf(BT_SHELL_ERROR, fmt, args);
	va_end(args);
}

int bt_shell_cmd_register(struct bt_shell *sh, struct bt_shell_cmd *cmd)
{
	struct bt_shell_cmd *tmp;

	BT_SLIST_FOR_EACH_CONTAINER(&sh->cmd_list, tmp, node) {
		if (tmp == cmd) {
			return -EEXIST;
		}
	}

	bt_slist_append(&sh->cmd_list, &cmd->node);

	return 0;
}

int bt_shell_cmd_unregister(const struct bt_shell_cmd *cmd)
{
	// not implemented
	return 0;
}

void bt_shell_help(const struct bt_shell *sh)
{
	const struct bt_shell_cmd_entry *pcmds = &sh->active_cmd;

	bt_shell_fprintf_print("Help message\n");
	bt_shell_fprintf_info("\t%s mands:%d opts:%d help:%s\n", pcmds->syntax,
			      pcmds->args.mandatory, pcmds->args.optional, pcmds->help);

	if (!pcmds->subcmd) {
		return;
	}

	pcmds = pcmds->subcmd;
	for (; pcmds && pcmds->syntax; pcmds++) {
		bt_shell_fprintf_info("\t%s mands:%d opts:%d help:%s\n", pcmds->syntax,
				      pcmds->args.mandatory, pcmds->args.optional, pcmds->help);
	}
}

void bt_shell_cmds_show(const struct bt_shell *sh)
{
	struct bt_shell_cmd *cmd;

	BT_SLIST_FOR_EACH_CONTAINER(&sh->cmd_list, cmd, node) {
		bt_shell_fprintf_info("%s\t%s\n", cmd->cmd_entry->syntax, cmd->cmd_entry->help);
	}
}

static const struct bt_shell_cmd_entry *bt_shell_root_cmd_find(struct bt_shell *sh,
							       const char *name)
{
	const struct bt_shell_cmd *cmd;

	BT_SLIST_FOR_EACH_CONTAINER(&sh->cmd_list, cmd, node) {
		if (!strcmp(cmd->cmd_entry->syntax, name)) {
			return cmd->cmd_entry;
		}
	}

	return NULL;
}

int bt_shell_exec(struct bt_shell *sh, const char *argv[], int argc)
{
	const struct bt_shell_cmd_entry *cmd;

	cmd = bt_shell_root_cmd_find(sh, argv[0]);
	if (!cmd) {
		return -ENOEXEC;
	}

	if (argc == 1) {
		memcpy(&sh->active_cmd, cmd, sizeof(struct bt_shell_cmd_entry));

		if (!cmd->handler) {
			return 0;
		}

		return cmd->handler(sh, argc, (char **)&argv[0]);
	}

	cmd = cmd->subcmd;
	for (; cmd && cmd->syntax; cmd++) {
		if (strcmp(argv[1], cmd->syntax)) {
			continue;
		}

		if (cmd->args.mandatory > argc - 1) {
			bt_shell_fprintf_info("cmd:%s Mands:%d opts:%d help:%s\n", cmd->syntax,
					      cmd->args.mandatory, cmd->args.optional, cmd->help);
			return 0;
		}

		memcpy(&sh->active_cmd, cmd, sizeof(struct bt_shell_cmd_entry));
		return cmd->handler(sh, argc - 1, (char **)&argv[1]);
	}

	return -ENOEXEC;
}

void bt_shell_init(struct bt_shell *sh)
{
	bt_slist_init(&sh->cmd_list);

	/* 基础蓝牙功能 */
	bt_shell_cmd_bt_register(sh);

	/* 经典蓝牙功能 */
#ifdef CONFIG_BT_CLASSIC
	bt_shell_cmd_br_register(sh);
	/* A2DP */
#ifdef CONFIG_BT_A2DP
	bt_shell_cmd_a2dp_register(sh);
#endif

	/* AVRCP */
#ifdef CONFIG_BT_AVRCP
	bt_shell_cmd_avrcp_register(sh);
#endif

	/* RFCOMM */
#ifdef CONFIG_BT_RFCOMM
	bt_shell_cmd_rfcomm_register(sh);
#endif

	/* HFP */
#if defined(CONFIG_BT_HFP_HF) || defined(CONFIG_BT_HFP_AG)
	bt_shell_cmd_hfp_register(sh);
#endif

	/* GOEP */
#ifdef CONFIG_BT_GOEP
	bt_shell_cmd_goep_register(sh);
#endif
#endif

	/* L2CAP */
#ifdef CONFIG_BT_L2CAP_DYNAMIC_CHANNEL
	bt_shell_cmd_l2cap_register(sh);
#endif

	/* GATT */
#ifdef CONFIG_BT_CONN
	bt_shell_cmd_gatt_register(sh);
#endif

	/* 信道探测 */
#ifdef CONFIG_BT_CHANNEL_SOUNDING
	bt_shell_cmd_cs_register(sh);
#endif

	/* ISO */
#ifdef CONFIG_BT_ISO
	bt_shell_cmd_iso_register(sh);
#endif

	/* IAS */
#ifdef CONFIG_BT_IAS
	bt_shell_cmd_ias_register(sh);
#endif

	/* Mesh */
#ifdef CONFIG_BT_MESH
#ifdef CONFIG_BT_MESH_SHELL
	bt_shell_cmd_mesh_register(sh);
#endif
#endif

	/* IAS客户端 */
#ifdef CONFIG_BT_IAS_CLIENT
	bt_shell_cmd_ias_client_register(sh);
#endif

	/* 音频服务 - 音量控制 */
#ifdef CONFIG_BT_VCP_VOL_REND
	bt_shell_cmd_vcp_vol_rend_register(sh);
#endif

#ifdef CONFIG_BT_VCP_VOL_CTLR
	bt_shell_cmd_vcp_vol_ctlr_register(sh);
#endif

	/* TMAP */
#ifdef CONFIG_BT_TMAP
	bt_shell_cmd_tmap_register(sh);
#endif

	/* TBS */
#ifdef CONFIG_BT_TBS
	bt_shell_cmd_tbs_register(sh);
#endif

#ifdef CONFIG_BT_TBS_CLIENT
	bt_shell_cmd_tbs_client_register(sh);
#endif

	/* PBP */
#ifdef CONFIG_BT_PBP
	bt_shell_cmd_pbp_register(sh);
#endif

	/* MPL */
#ifdef CONFIG_BT_MPL
	bt_shell_cmd_mpl_register(sh);
#endif

	/* MICP */
#ifdef CONFIG_BT_MICP_MIC_DEV
	bt_shell_cmd_micp_mic_dev_register(sh);
#endif

#ifdef CONFIG_BT_MICP_MIC_CTLR
	bt_shell_cmd_micp_mic_ctlr_register(sh);
#endif

	/* 媒体控制 */
#ifdef CONFIG_BT_MCC
	bt_shell_cmd_mcc_register(sh);
#endif

#ifdef CONFIG_BT_MCTL
	bt_shell_cmd_media_register(sh);
#endif

	/* HAS */
#ifdef CONFIG_BT_HAS
	bt_shell_cmd_has_register(sh);
#endif

#ifdef CONFIG_BT_HAS_CLIENT
	bt_shell_cmd_has_client_register(sh);
#endif

	/* GMAP */
#ifdef CONFIG_BT_GMAP
	bt_shell_cmd_gmap_register(sh);
#endif

	/* CSIP */
#ifdef CONFIG_BT_CSIP_SET_MEMBER
	bt_shell_cmd_csip_set_member_register(sh);
#endif

#ifdef CONFIG_BT_CSIP_SET_COORDINATOR
	bt_shell_cmd_csip_set_coordinator_register(sh);
#endif

	/* CCP */
#ifdef CONFIG_BT_CCAP_CALL_CONTROL_SERVER
	bt_shell_cmd_ccp_call_control_server_register(sh);
#endif

#ifdef CONFIG_BT_CCAP_CALL_CONTROL_CLIENT
	bt_shell_cmd_ccp_call_control_client_register(sh);
#endif

	/* CAP */
#ifdef CONFIG_BT_CAP_ACCEPTOR
	bt_shell_cmd_cap_acceptor_register(sh);
#endif

#ifdef CONFIG_BT_CAP_COMMANDER
	bt_shell_cmd_cap_commander_register(sh);
#endif

#ifdef CONFIG_BT_CAP_INITIATOR
	bt_shell_cmd_cap_initiator_register(sh);
#endif

	/* BAP */
#ifdef CONFIG_BT_BAP_UNICAST_CLIENT
	bt_shell_cmd_bap_register(sh);
#endif

#ifdef CONFIG_BT_BAP_SCAN_DELEGATOR
	bt_shell_cmd_bap_scan_delegator_register(sh);
#endif

#ifdef CONFIG_BT_BAP_BROADCAST_ASSISTANT
	bt_shell_cmd_bap_broadcast_assistant_register(sh);
#endif
}

void bt_shell_uninit(struct bt_shell *sh)
{
	// not implemented
}