/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_SHELL_PRIVATE_H
#define __BT_SHELL_PRIVATE_H

#include <stdint.h>

#include <utils/bt_slist.h>

#define BT_SHELL_PRINT  "PRINT"
#define BT_SHELL_NORMAL "NORMAL"
#define BT_SHELL_INFO   "INFO"
#define BT_SHELL_WARN   "WARN"
#define BT_SHELL_ERROR  "ERROR"

#define BT_SHELL_CMD_HELP_PRINTED 0

#define BT_SHELL_SUBCMD_SET_END {0}

struct bt_shell;

typedef int (*bt_shell_cmd_handler_t)(const struct bt_shell *sh, size_t argc, char **argv);

struct bt_shell_args {
	uint8_t mandatory; /*!< Number of mandatory arguments. */
	uint8_t optional;  /*!< Number of optional arguments. */
};

struct bt_shell_cmd_entry {
	const char *syntax;                      /*!< Command syntax strings. */
	const char *help;                        /*!< Command help string. */
	const struct bt_shell_cmd_entry *subcmd; /*!< Pointer to subcommand. */
	bt_shell_cmd_handler_t handler;          /*!< Command handler. */
	struct bt_shell_args args;               /*!< Command arguments. */
	uint8_t padding[0];
};

struct bt_shell_cmd {
	const struct bt_shell_cmd_entry *cmd_entry;
	bt_snode_t node;
};

struct bt_shell {
	bt_slist_t cmd_list;                  /*!< List of registered shell commands. */
	struct bt_shell_cmd_entry active_cmd; /*!< Active command. */
	void *user_data;                      /*!< User data. */
};

#define BT_SHELL_CMD_ARG(_syntax, _subcmd, _help, _handler, _mand, _opt)                           \
	{                                                                                          \
		.syntax = (const char *)UTILS_STRINGIFY(_syntax), .help = (const char *)_help,     \
		.subcmd = (const struct bt_shell_cmd_entry *)_subcmd,                              \
		.handler = (bt_shell_cmd_handler_t)_handler, .args = {                             \
			.mandatory = _mand,                                                        \
			.optional = _opt                                                           \
		}                                                                                  \
	}

#define BT_SHELL_CMD(_syntax, _subcmd, _help, _handler)                                            \
	BT_SHELL_CMD_ARG(_syntax, _subcmd, _help, _handler, 0, 0)

#define BT_SHELL_CMD_ARG_DEFINE(_syntax, _subcmd, _help, _handler, _mand, _opt)                  \
	static const struct bt_shell_cmd_entry _syntax##_entry =                                   \
		BT_SHELL_CMD_ARG(_syntax, _subcmd, _help, _handler, _mand, _opt);                  \
	static struct bt_shell_cmd _syntax = {.cmd_entry = &_syntax##_entry, .node = {.next = NULL}}

#define BT_SHELL_CMD_DEFINE(_syntax, _subcmd, _help, _handler)                                   \
	BT_SHELL_CMD_ARG_DEFINE(_syntax, _subcmd, _help, _handler, 0, 0)

#define BT_SHELL_SUBCMD_SET_CREATE(name, ...)                                               \
	static const struct bt_shell_cmd_entry name[] = {__VA_ARGS__};

long bt_shell_strtol(const char *str, int base, int *err);
unsigned long bt_shell_strtoul(const char *str, int base, int *err);
unsigned long long bt_shell_strtoull(const char *str, int base, int *err);
bool bt_shell_strtobool(const char *str, int base, int *err);
void bt_shell_help(const struct bt_shell *sh);

/**
 * @brief printf-like function which sends formatted data stream to the shell.
 * (Bluetooth context specific)
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context.
 *
 * @param[in] fmt   Format string.
 * @param[in] ...   List of parameters to print.
 */
void bt_shell_fprintf(const char *fmt, ...);

/**
 * @brief printf-like function which sends formatted data stream to the shell.
 * (Bluetooth context specific)
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context.
 *
 * @param[in] fmt   Format string.
 * @param[in] ...   List of parameters to print.
 */
void bt_shell_fprintf_info(const char *fmt, ...);
void bt_shell_fprintf_print(const char *fmt, ...);
void bt_shell_fprintf_warn(const char *fmt, ...);
void bt_shell_fprintf_error(const char *fmt, ...);

/**
 * @brief Print data in hexadecimal format.
 * (Bluetooth context specific)
 *
 * @param[in] data Pointer to data.
 * @param[in] len  Length of data.
 */
void bt_shell_hexdump(const uint8_t *data, size_t len);

/**
 * @brief Print info message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_info(_ft, ...) bt_shell_fprintf_info(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print normal message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_print(_ft, ...) bt_shell_fprintf_print(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print warning message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_warn(_ft, ...) bt_shell_fprintf_warn(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print error message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_error(_ft, ...) bt_shell_fprintf_error(_ft "\n", ##__VA_ARGS__)

int bt_shell_cmd_register(struct bt_shell *sh, struct bt_shell_cmd *cmd);
int bt_shell_cmd_unregister(const struct bt_shell_cmd *cmd);
void bt_shell_help(const struct bt_shell *sh);
void bt_shell_cmds_show(const struct bt_shell *sh);
int bt_shell_exec(struct bt_shell *sh, const char *argv[], int argc);
void bt_shell_init(struct bt_shell *sh);
void bt_shell_uninit(struct bt_shell *sh);

extern int bt_shell_cmd_bt_register(struct bt_shell *sh);
extern int bt_shell_cmd_br_register(struct bt_shell *sh);
extern int bt_shell_cmd_l2cap_register(struct bt_shell *sh);
extern int bt_shell_cmd_gatt_register(struct bt_shell *sh);
extern int bt_shell_cmd_a2dp_register(struct bt_shell *sh);
extern int bt_shell_cmd_avrcp_register(struct bt_shell *sh);
extern int bt_shell_cmd_rfcomm_register(struct bt_shell *sh);
extern int bt_shell_cmd_hfp_register(struct bt_shell *sh);
extern int bt_shell_cmd_goep_register(struct bt_shell *sh);
extern int bt_shell_cmd_cs_register(struct bt_shell *sh);
extern int bt_shell_cmd_iso_register(struct bt_shell *sh);
extern int bt_shell_cmd_ias_register(struct bt_shell *sh);
extern int bt_shell_cmd_mesh_register(struct bt_shell *sh);
extern int bt_shell_cmd_ias_client_register(struct bt_shell *sh);
extern int bt_shell_cmd_vcp_vol_rend_register(struct bt_shell *sh);
extern int bt_shell_cmd_vcp_vol_ctlr_register(struct bt_shell *sh);
extern int bt_shell_cmd_tmap_register(struct bt_shell *sh);
extern int bt_shell_cmd_tbs_register(struct bt_shell *sh);
extern int bt_shell_cmd_tbs_client_register(struct bt_shell *sh);
extern int bt_shell_cmd_pbp_register(struct bt_shell *sh);
extern int bt_shell_cmd_mpl_register(struct bt_shell *sh);
extern int bt_shell_cmd_micp_mic_dev_register(struct bt_shell *sh);
extern int bt_shell_cmd_micp_mic_ctlr_register(struct bt_shell *sh);
extern int bt_shell_cmd_media_register(struct bt_shell *sh);
extern int bt_shell_cmd_mcc_register(struct bt_shell *sh);
extern int bt_shell_cmd_has_register(struct bt_shell *sh);
extern int bt_shell_cmd_has_client_register(struct bt_shell *sh);
extern int bt_shell_cmd_gmap_register(struct bt_shell *sh);
extern int bt_shell_cmd_csip_set_member_register(struct bt_shell *sh);
extern int bt_shell_cmd_csip_set_coordinator_register(struct bt_shell *sh);
extern int bt_shell_cmd_ccp_call_control_server_register(struct bt_shell *sh);
extern int bt_shell_cmd_ccp_call_control_client_register(struct bt_shell *sh);
extern int bt_shell_cmd_cap_initiator_register(struct bt_shell *sh);
extern int bt_shell_cmd_cap_commander_register(struct bt_shell *sh);
extern int bt_shell_cmd_cap_acceptor_register(struct bt_shell *sh);
extern int bt_shell_cmd_bap_register(struct bt_shell *sh);
extern int bt_shell_cmd_bap_scan_delegator_register(struct bt_shell *sh);
extern int bt_shell_cmd_bap_broadcast_assistant_register(struct bt_shell *sh);

#endif /* __BT_SHELL_PRIVATE_H */
