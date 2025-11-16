/** @file
 *  @brief Media Control Client shell implementation
 *
 */

/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bluetooth/audio/mcc.h>
#include <bluetooth/audio/mcs.h>
#include <bluetooth/audio/media_proxy.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/ots.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#include "../media_proxy_internal.h"


static struct bt_mcc_cb cb;

#ifdef CONFIG_BT_MCC_OTS
struct object_ids_t {
	uint64_t icon_obj_id;
	uint64_t track_segments_obj_id;
	uint64_t current_track_obj_id;
	uint64_t next_track_obj_id;
	uint64_t parent_group_obj_id;
	uint64_t current_group_obj_id;
	uint64_t search_results_obj_id;
};
static struct object_ids_t obj_ids;
#endif /* CONFIG_BT_MCC_OTS */


static void mcc_discover_mcs_cb(struct bt_conn *conn, int err)
{
	if (err) {
		bt_shell_error("Discovery failed (%d)", err);
		return;
	}

	bt_shell_print("Discovery complete");
}

static void mcc_read_player_name_cb(struct bt_conn *conn, int err, const char *name)
{
	if (err) {
		bt_shell_error("Player Name read failed (%d)", err);
		return;
	}

	bt_shell_print("Player name: %s", name);
}

#ifdef CONFIG_BT_MCC_OTS
static void mcc_read_icon_obj_id_cb(struct bt_conn *conn, int err, uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Icon Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Icon object ID: %s", str);

	obj_ids.icon_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
static void mcc_read_icon_url_cb(struct bt_conn *conn, int err, const char *url)
{
	if (err) {
		bt_shell_error("Icon URL read failed (%d)", err);
		return;
	}

	bt_shell_print("Icon URL: 0x%s", url);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
static void mcc_read_track_title_cb(struct bt_conn *conn, int err, const char *title)
{
	if (err) {
		bt_shell_error("Track title read failed (%d)", err);
		return;
	}

	bt_shell_print("Track title: %s", title);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */

static void mcc_track_changed_ntf_cb(struct bt_conn *conn, int err)
{
	if (err) {
		bt_shell_error("Track changed notification failed (%d)", err);
		return;
	}

	bt_shell_print("Track changed");
}

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
static void mcc_read_track_duration_cb(struct bt_conn *conn, int err, int32_t dur)
{
	if (err) {
		bt_shell_error("Track duration read failed (%d)", err);
		return;
	}

	bt_shell_print("Track duration: %d", dur);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
static void mcc_read_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		bt_shell_error("Track position read failed (%d)", err);
		return;
	}

	bt_shell_print("Track Position: %d", pos);
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
static void mcc_set_track_position_cb(struct bt_conn *conn, int err, int32_t pos)
{
	if (err) {
		bt_shell_error("Track Position set failed (%d)", err);
		return;
	}

	bt_shell_print("Track Position: %d", pos);
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
static void mcc_read_playback_speed_cb(struct bt_conn *conn, int err,
				       int8_t speed)
{
	if (err) {
		bt_shell_error("Playback speed read failed (%d)", err);
		return;
	}

	bt_shell_print("Playback speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
static void mcc_set_playback_speed_cb(struct bt_conn *conn, int err, int8_t speed)
{
	if (err) {
		bt_shell_error("Playback speed set failed (%d)", err);
		return;
	}

	bt_shell_print("Playback speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
static void mcc_read_seeking_speed_cb(struct bt_conn *conn, int err,
				      int8_t speed)
{
	if (err) {
		bt_shell_error("Seeking speed read failed (%d)", err);
		return;
	}

	bt_shell_print("Seeking speed: %d", speed);
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */


#ifdef CONFIG_BT_MCC_OTS
static void mcc_read_segments_obj_id_cb(struct bt_conn *conn, int err,
					uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Track Segments Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Track Segments Object ID: %s", str);

	obj_ids.track_segments_obj_id = id;
}


static void mcc_read_current_track_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Current Track Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Current Track Object ID: %s", str);

	obj_ids.current_track_obj_id = id;
}


static void mcc_set_current_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Current Track Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Current Track Object ID written: %s", str);
}


static void mcc_read_next_track_obj_id_cb(struct bt_conn *conn, int err,
					  uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Next Track Object ID read failed (%d)", err);
		return;
	}

	if (id == MPL_NO_TRACK_ID) {
		bt_shell_print("Next Track Object ID is empty");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		bt_shell_print("Next Track Object ID: %s", str);
	}

	obj_ids.next_track_obj_id = id;
}


static void mcc_set_next_track_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Next Track Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Next Track Object ID written: %s", str);
}


static void mcc_read_parent_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Parent Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Parent Group Object ID: %s", str);

	obj_ids.parent_group_obj_id = id;
}


static void mcc_read_current_group_obj_id_cb(struct bt_conn *conn, int err,
					     uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Current Group Object ID read failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Current Group Object ID: %s", str);

	obj_ids.current_group_obj_id = id;
}

static void mcc_set_current_group_obj_id_cb(struct bt_conn *conn, int err,
					    uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Current Group Object ID set failed (%d)", err);
		return;
	}

	(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
	bt_shell_print("Current Group Object ID written: %s", str);
}
#endif /* CONFIG_BT_MCC_OTS */


#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
static void mcc_read_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		bt_shell_error("Playing order read failed (%d)", err);
		return;
	}

	bt_shell_print("Playing order: %d", order);
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
static void mcc_set_playing_order_cb(struct bt_conn *conn, int err, uint8_t order)
{
	if (err) {
		bt_shell_error("Playing order set failed (%d)", err);
		return;
	}

	bt_shell_print("Playing order: %d", order);
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
static void mcc_read_playing_orders_supported_cb(struct bt_conn *conn, int err,
						 uint16_t orders)
{
	if (err) {
		bt_shell_error("Playing orders supported read failed (%d)", err);
		return;
	}

	bt_shell_print("Playing orders supported: %d", orders);
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
static void mcc_read_media_state_cb(struct bt_conn *conn, int err, uint8_t state)
{
	if (err) {
		bt_shell_error("Media State read failed (%d)", err);
		return;
	}

	bt_shell_print("Media State: %d", state);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
static void mcc_send_cmd_cb(struct bt_conn *conn, int err, const struct mpl_cmd *cmd)
{
	if (err) {
		bt_shell_error("Command send failed (%d) - opcode: %d, param: %d",
			       err, cmd->opcode, cmd->param);
		return;
	}

	bt_shell_print("Command opcode: %d, param: %d", cmd->opcode, cmd->param);
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

static void mcc_cmd_ntf_cb(struct bt_conn *conn, int err,
			   const struct mpl_cmd_ntf *ntf)
{
	if (err) {
		bt_shell_error("Command notification error (%d) - opcode: %d, result: %d",
			       err, ntf->requested_opcode, ntf->result_code);
		return;
	}

	bt_shell_print("Command opcode: %d, result: %d",
		       ntf->requested_opcode, ntf->result_code);
}

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
static void mcc_read_opcodes_supported_cb(struct bt_conn *conn, int err,
					    uint32_t opcodes)
{
	if (err) {
		bt_shell_error("Opcodes supported read failed (%d)", err);
		return;
	}

	bt_shell_print("Opcodes supported: %d", opcodes);
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
static void mcc_send_search_cb(struct bt_conn *conn, int err,
			       const struct mpl_search *search)
{
	if (err) {
		bt_shell_error("Search send failed (%d)", err);
		return;
	}

	bt_shell_print("Search sent");
}

static void mcc_search_ntf_cb(struct bt_conn *conn, int err, uint8_t result_code)
{
	if (err) {
		bt_shell_error("Search notification error (%d), result code: %d",
			       err, result_code);
		return;
	}

	bt_shell_print("Search notification result code: %d", result_code);
}

static void mcc_read_search_results_obj_id_cb(struct bt_conn *conn, int err,
					      uint64_t id)
{
	char str[BT_OTS_OBJ_ID_STR_LEN];

	if (err) {
		bt_shell_error("Search Results Object ID read failed (%d)", err);
		return;
	}

	if (id == 0) {
		bt_shell_print("Search Results Object ID: 0x000000000000");
	} else {
		(void)bt_ots_obj_id_to_str(id, str, sizeof(str));
		bt_shell_print("Search Results Object ID: %s", str);
	}

	obj_ids.search_results_obj_id = id;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
static void mcc_read_content_control_id_cb(struct bt_conn *conn, int err, uint8_t ccid)
{
	if (err) {
		bt_shell_error("Content Control ID read failed (%d)", err);
		return;
	}

	bt_shell_print("Content Control ID: %d", ccid);
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */

#ifdef CONFIG_BT_MCC_OTS
/**** Callback functions for the included Object Transfer service *************/
static void mcc_otc_obj_selected_cb(struct bt_conn *conn, int err)
{
	if (err) {
		bt_shell_error("Error in selecting object (err %d)", err);
		return;
	}

	bt_shell_print("Selecting object succeeded");
}

static void mcc_otc_obj_metadata_cb(struct bt_conn *conn, int err)
{
	if (err) {
		bt_shell_error("Error in reading object metadata (err %d)", err);
		return;
	}

	bt_shell_print("Reading object metadata succeeded\n");
}

static void mcc_icon_object_read_cb(struct bt_conn *conn, int err,
				    struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Icon Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Icon content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

/* TODO: May want to use a parsed type, instead of the raw buf, here */
static void mcc_track_segments_object_read_cb(struct bt_conn *conn, int err,
					      struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Track Segments Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Track Segments content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void mcc_otc_read_current_track_object_cb(struct bt_conn *conn, int err,
						 struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Current Track Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Current Track content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void mcc_otc_read_next_track_object_cb(struct bt_conn *conn, int err,
					      struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Next Track Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Next Track content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void mcc_otc_read_parent_group_object_cb(struct bt_conn *conn, int err,
						struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Parent Group Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Parent Group content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

static void mcc_otc_read_current_group_object_cb(struct bt_conn *conn, int err,
						 struct bt_buf_simple *buf)
{
	if (err) {
		bt_shell_error("Current Group Object read failed (%d)", err);
		return;
	}

	bt_shell_print("Current Group content (%d octets)", buf->len);
	bt_shell_hexdump(buf->data, buf->len);
}

#endif /* CONFIG_BT_MCC_OTS */


static int cmd_mcc_init(const struct bt_shell *sh, size_t argc, char **argv)
{
	int result;

	/* Set up the callbacks */
	cb.discover_mcs                  = mcc_discover_mcs_cb;
	cb.read_player_name              = mcc_read_player_name_cb;
#ifdef CONFIG_BT_MCC_OTS
	cb.read_icon_obj_id              = mcc_read_icon_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
	cb.read_icon_url                 = mcc_read_icon_url_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */
	cb.track_changed_ntf             = mcc_track_changed_ntf_cb;
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
	cb.read_track_title              = mcc_read_track_title_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	cb.read_track_duration           = mcc_read_track_duration_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	cb.read_track_position           = mcc_read_track_position_cb;
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
	cb.set_track_position            = mcc_set_track_position_cb;
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	cb.read_playback_speed           = mcc_read_playback_speed_cb;
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
	cb.set_playback_speed            = mcc_set_playback_speed_cb;
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	cb.read_seeking_speed            = mcc_read_seeking_speed_cb;
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
	cb.read_segments_obj_id          = mcc_read_segments_obj_id_cb;
	cb.read_current_track_obj_id     = mcc_read_current_track_obj_id_cb;
	cb.set_current_track_obj_id      = mcc_set_current_track_obj_id_cb;
	cb.read_next_track_obj_id        = mcc_read_next_track_obj_id_cb;
	cb.set_next_track_obj_id         = mcc_set_next_track_obj_id_cb;
	cb.read_parent_group_obj_id      = mcc_read_parent_group_obj_id_cb;
	cb.read_current_group_obj_id     = mcc_read_current_group_obj_id_cb;
	cb.set_current_group_obj_id      = mcc_set_current_group_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	cb.read_playing_order            = mcc_read_playing_order_cb;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
	cb.set_playing_order             = mcc_set_playing_order_cb;
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
	cb.read_playing_orders_supported = mcc_read_playing_orders_supported_cb;
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	cb.read_media_state              = mcc_read_media_state_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
	cb.send_cmd                      = mcc_send_cmd_cb;
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */
	cb.cmd_ntf                       = mcc_cmd_ntf_cb;
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	cb.read_opcodes_supported        = mcc_read_opcodes_supported_cb;
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
	cb.send_search                   = mcc_send_search_cb;
	cb.search_ntf                    = mcc_search_ntf_cb;
	cb.read_search_results_obj_id    = mcc_read_search_results_obj_id_cb;
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
	cb.read_content_control_id       = mcc_read_content_control_id_cb;
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */
#ifdef CONFIG_BT_MCC_OTS
	cb.otc_obj_selected              = mcc_otc_obj_selected_cb;
	cb.otc_obj_metadata              = mcc_otc_obj_metadata_cb;
	cb.otc_icon_object               = mcc_icon_object_read_cb;
	cb.otc_track_segments_object     = mcc_track_segments_object_read_cb;
	cb.otc_current_track_object      = mcc_otc_read_current_track_object_cb;
	cb.otc_next_track_object         = mcc_otc_read_next_track_object_cb;
	cb.otc_parent_group_object       = mcc_otc_read_parent_group_object_cb;
	cb.otc_current_group_object      = mcc_otc_read_current_group_object_cb;
#endif /* CONFIG_BT_MCC_OTS */

	/* Initialize the module */
	result = bt_mcc_init(&cb);

	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_discover_mcs(const struct bt_shell *sh, size_t argc,
				char **argv)
{
	bool subscribe = true;
	int result = 0;

	if (argc > 1) {
		subscribe = bt_shell_strtobool(argv[1], 0, &result);
		if (result != 0) {
			bt_shell_error("Could not parse subscribe: %d",
				    result);

			return -ENOEXEC;
		}
	}

	result = bt_mcc_discover_mcs(default_conn, (bool)subscribe);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_player_name(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_player_name(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_read_icon_obj_id(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_icon_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
static int cmd_mcc_read_icon_url(const struct bt_shell *sh, size_t argc,
				 char *argv[])
{
	int result;

	result = bt_mcc_read_icon_url(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */

#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
static int cmd_mcc_read_track_title(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_track_title(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */

#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
static int cmd_mcc_read_track_duration(const struct bt_shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_track_duration(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */

#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
static int cmd_mcc_read_track_position(const struct bt_shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_track_position(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */

#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
static int cmd_mcc_set_track_position(const struct bt_shell *sh, size_t argc,
				      char *argv[])
{
	int result = 0;
	long pos;

	pos = bt_shell_strtol(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse pos: %d", result);

		return -ENOEXEC;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(pos, INT32_MIN, INT32_MAX)) {
		bt_shell_error("Invalid pos: %ld", pos);

		return -ENOEXEC;
	}

	result = bt_mcc_set_track_position(default_conn, pos);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */


#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
static int cmd_mcc_read_playback_speed(const struct bt_shell *sh, size_t argc,
				       char *argv[])
{
	int result;

	result = bt_mcc_read_playback_speed(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED) */


#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
static int cmd_mcc_set_playback_speed(const struct bt_shell *sh, size_t argc,
				      char *argv[])
{
	int result = 0;
	long speed;

	speed = bt_shell_strtol(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse speed: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(speed, INT8_MIN, INT8_MAX)) {
		bt_shell_error("Invalid speed: %ld", speed);

		return -ENOEXEC;
	}

	result = bt_mcc_set_playback_speed(default_conn, speed);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */

#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
static int cmd_mcc_read_seeking_speed(const struct bt_shell *sh, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_seeking_speed(default_conn);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */


#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_read_track_segments_obj_id(const struct bt_shell *sh,
					      size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_segments_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}


static int cmd_mcc_read_current_track_obj_id(const struct bt_shell *sh,
					     size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_current_track_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_current_track_obj_id(const struct bt_shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = bt_shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		bt_shell_error("Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_current_track_obj_id(default_conn, id);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_next_track_obj_id(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	int result;

	result = bt_mcc_read_next_track_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_next_track_obj_id(const struct bt_shell *sh, size_t argc,
					 char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = bt_shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		bt_shell_error("Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_next_track_obj_id(default_conn, id);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_parent_group_obj_id(const struct bt_shell *sh, size_t argc,
					    char *argv[])
{
	int result;

	result = bt_mcc_read_parent_group_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_read_current_group_obj_id(const struct bt_shell *sh,
					     size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_current_group_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_set_current_group_obj_id(const struct bt_shell *sh, size_t argc,
					    char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = bt_shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		bt_shell_error("Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_mcc_set_current_group_obj_id(default_conn, id);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
static int cmd_mcc_read_playing_order(const struct bt_shell *sh, size_t argc,
				      char *argv[])
{
	int result;

	result = bt_mcc_read_playing_order(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
static int cmd_mcc_set_playing_order(const struct bt_shell *sh, size_t argc,
				     char *argv[])
{
	unsigned long order;
	int result = 0;

	order = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse order: %d", result);

		return -ENOEXEC;
	}

	if (order > UINT8_MAX) {
		bt_shell_error("Invalid order: %lu", order);

		return -ENOEXEC;
	}

	result = bt_mcc_set_playing_order(default_conn, order);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */

#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
static int cmd_mcc_read_playing_orders_supported(const struct bt_shell *sh,
						 size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_playing_orders_supported(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
static int cmd_mcc_read_media_state(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_mcc_read_media_state(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */

#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
static int cmd_mcc_play(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PLAY,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC play failed: %d", err);
	}

	return err;
}

static int cmd_mcc_pause(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PAUSE,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC pause failed: %d", err);
	}

	return err;
}

static int cmd_mcc_fast_rewind(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FAST_REWIND,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC fast rewind failed: %d", err);
	}

	return err;
}

static int cmd_mcc_fast_forward(const struct bt_shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FAST_FORWARD,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC fast forward failed: %d", err);
	}

	return err;
}

static int cmd_mcc_stop(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_STOP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC stop failed: %d", err);
	}

	return err;
}

static int cmd_mcc_move_relative(const struct bt_shell *sh, size_t argc,
				 char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_MOVE_RELATIVE,
		.use_param = true,
	};
	long offset;
	int err;

	err = 0;
	offset = bt_shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		bt_shell_error("Failed to parse offset: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(offset, INT32_MIN, INT32_MAX)) {
		bt_shell_error("Invalid offset: %ld", offset);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)offset;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC move relative failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_segment(const struct bt_shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC previous segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_segment(const struct bt_shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC next segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_segment(const struct bt_shell *sh, size_t argc,
				 char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC first segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_segment(const struct bt_shell *sh, size_t argc,
				char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_SEGMENT,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC last segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_segment(const struct bt_shell *sh, size_t argc,
				char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_SEGMENT,
		.use_param = true,
	};
	long segment;
	int err;

	err = 0;
	segment = bt_shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		bt_shell_error("Failed to parse segment: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(segment, INT32_MIN, INT32_MAX)) {
		bt_shell_error("Invalid segment: %ld", segment);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)segment;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC goto segment failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_track(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC previous track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_track(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC next track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_track(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC first track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_track(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_TRACK,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC last track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_track(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_TRACK,
		.use_param = true,
	};
	long track;
	int err;

	err = 0;
	track = bt_shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		bt_shell_error("Failed to parse track: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(track, INT32_MIN, INT32_MAX)) {
		bt_shell_error("Invalid track: %ld", track);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)track;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC goto track failed: %d", err);
	}

	return err;
}

static int cmd_mcc_prev_group(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_PREV_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC previous group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_next_group(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_NEXT_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC next group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_first_group(const struct bt_shell *sh, size_t argc,
			       char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_FIRST_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC first group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_last_group(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_LAST_GROUP,
		.use_param = false,
		.param = 0,
	};
	int err;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC last group failed: %d", err);
	}

	return err;
}

static int cmd_mcc_goto_group(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct mpl_cmd cmd = {
		.opcode = BT_MCS_OPC_GOTO_GROUP,
		.use_param = true,
	};
	long group;
	int err;

	err = 0;
	group = bt_shell_strtol(argv[1], 10, &err);
	if (err != 0) {
		bt_shell_error("Failed to parse group: %d", err);

		return err;
	}

	if (sizeof(long) != sizeof(int32_t) && !IN_RANGE(group, INT32_MIN, INT32_MAX)) {
		bt_shell_error("Invalid group: %ld", group);

		return -ENOEXEC;
	}

	cmd.param = (int32_t)group;

	err = bt_mcc_send_cmd(default_conn, &cmd);
	if (err != 0) {
		bt_shell_error("MCC goto group failed: %d", err);
	}

	return err;
}
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */

#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
static int cmd_mcc_read_opcodes_supported(const struct bt_shell *sh, size_t argc,
					  char *argv[])
{
	int result;

	result = bt_mcc_read_opcodes_supported(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */

#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_send_search_raw(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	int result;
	size_t len;
	struct mpl_search search;

	len = strlen(argv[1]);
	if (len > sizeof(search.search)) {
		bt_shell_print("Fail: Invalid argument");
		return -EINVAL;
	}

	search.len = len;
	memcpy(search.search, argv[1], search.len);
	LOG_DBG("Search string: %s", argv[1]);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_send_search_ioptest(const struct bt_shell *sh, size_t argc,
				       char *argv[])
{
	/* Implementation follows Media control service testspec 0.9.0r13 */
	/* Testcase MCS/SR/SCP/BV-01-C [Search Control Point], rounds 1 - 9 */
	struct mpl_sci sci_1 = {0};
	struct mpl_sci sci_2 = {0};
	struct mpl_search search;
	unsigned long testround;
	int result = 0;

	testround = bt_shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse testround: %d", result);

		return -ENOEXEC;
	}

	switch (testround) {
	case 1:
	case 8:
	case 9:
		/* 1, 8 and 9 have the same first SCI */
		sci_1.type = BT_MCS_SEARCH_TYPE_TRACK_NAME;
		strcpy(sci_1.param, "TSPX_Track_Name");
		break;
	case 2:
		sci_1.type = BT_MCS_SEARCH_TYPE_ARTIST_NAME;
		strcpy(sci_1.param, "TSPX_Artist_Name");
		break;
	case 3:
		sci_1.type = BT_MCS_SEARCH_TYPE_ALBUM_NAME;
		strcpy(sci_1.param, "TSPX_Album_Name");
		break;
	case 4:
		sci_1.type = BT_MCS_SEARCH_TYPE_GROUP_NAME;
		strcpy(sci_1.param, "TSPX_Group_Name");
		break;
	case 5:
		sci_1.type = BT_MCS_SEARCH_TYPE_EARLIEST_YEAR;
		strcpy(sci_1.param, "TSPX_Earliest_Year");
		break;
	case 6:
		sci_1.type = BT_MCS_SEARCH_TYPE_LATEST_YEAR;
		strcpy(sci_1.param, "TSPX_Latest_Year");
		break;
	case 7:
		sci_1.type = BT_MCS_SEARCH_TYPE_GENRE;
		strcpy(sci_1.param, "TSPX_Genre");
		break;
	default:
		bt_shell_error("Invalid parameter");
		return -ENOEXEC;
	}


	switch (testround) {
	case 8:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_TRACKS;
		break;
	case 9:
		sci_2.type = BT_MCS_SEARCH_TYPE_ONLY_GROUPS;
		break;
	}

	/* Length is length of type, plus length of param w/o termination */
	sci_1.len = sizeof(sci_1.type) + strlen(sci_1.param);

	search.len = 0;
	memcpy(&search.search[search.len], &sci_1.len, sizeof(sci_1.len));
	search.len += sizeof(sci_1.len);

	memcpy(&search.search[search.len], &sci_1.type, sizeof(sci_1.type));
	search.len += sizeof(sci_1.type);

	memcpy(&search.search[search.len], &sci_1.param, strlen(sci_1.param));
	search.len += strlen(sci_1.param);

	if (testround == 8 || testround == 9) {

		sci_2.len = sizeof(sci_2.type); /* The type only, no param */

		memcpy(&search.search[search.len], &sci_2.len,
		       sizeof(sci_2.len));
		search.len += sizeof(sci_2.len);

		memcpy(&search.search[search.len], &sci_2.type,
		       sizeof(sci_2.type));
		search.len += sizeof(sci_2.type);
	}

	bt_shell_print("Search string: ");
	bt_shell_hexdump((uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_MCC_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
static int cmd_mcc_test_send_search_iop_invalid_type(const struct bt_shell *sh,
						     size_t argc, char *argv[])
{
	int result;
	struct mpl_search search;

	search.search[0] = 2;
	search.search[1] = (char)14; /* Invalid type value */
	search.search[2] = 't';  /* Anything */
	search.len = 3;

	bt_shell_print("Search string: ");
	bt_shell_hexdump((uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}

static int cmd_mcc_test_send_search_invalid_sci_len(const struct bt_shell *sh,
						    size_t argc, char *argv[])
{
	/* Reproduce a search that caused hard fault when sent from peer */
	/* in IOP testing */

	int result;
	struct mpl_search search;

	char offending_search[9] = {6, 1, 't', 'r', 'a', 'c', 'k', 0, 1 };

	search.len = 9;
	memcpy(&search.search, offending_search, search.len);

	bt_shell_print("Search string: ");
	bt_shell_hexdump((uint8_t *)&search.search, search.len);

	result = bt_mcc_send_search(default_conn, &search);
	if (result) {
		bt_shell_print("Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG && CONFIG_BT_TESTING */

static int cmd_mcc_read_search_results_obj_id(const struct bt_shell *sh,
					      size_t argc, char *argv[])
{
	int result;

	result = bt_mcc_read_search_results_obj_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
static int cmd_mcc_read_content_control_id(const struct bt_shell *sh, size_t argc,
					   char *argv[])
{
	int result;

	result = bt_mcc_read_content_control_id(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */


#ifdef CONFIG_BT_MCC_OTS
static int cmd_mcc_otc_read_features(const struct bt_shell *sh, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_ots_client_read_feature(bt_mcc_otc_inst(default_conn),
					    default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int result;

	result = bt_ots_client_read_object_data(bt_mcc_otc_inst(default_conn),
						default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_metadata(const struct bt_shell *sh, size_t argc,
				     char *argv[])
{
	int result;

	result = bt_ots_client_read_object_metadata(bt_mcc_otc_inst(default_conn),
						    default_conn,
						    BT_OTS_METADATA_REQ_ALL);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select(const struct bt_shell *sh, size_t argc, char *argv[])
{
	unsigned long long id;
	int result = 0;

	id = bt_shell_strtoull(argv[1], 0, &result);
	if (result != 0) {
		bt_shell_error("Could not parse id: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(id, BT_OTS_OBJ_ID_MIN, BT_OTS_OBJ_ID_MAX)) {
		bt_shell_error("Invalid id: %llu", id);

		return -ENOEXEC;
	}

	result = bt_ots_client_select_id(bt_mcc_otc_inst(default_conn),
					 default_conn, id);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_first(const struct bt_shell *sh, size_t argc,
				    char *argv[])
{
	int result;

	result = bt_ots_client_select_first(bt_mcc_otc_inst(default_conn),
					    default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_last(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_last(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_next(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_next(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_select_prev(const struct bt_shell *sh, size_t argc,
				   char *argv[])
{
	int result;

	result = bt_ots_client_select_prev(bt_mcc_otc_inst(default_conn),
					   default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_icon_object(const struct bt_shell *sh, size_t argc,
					char *argv[])
{
	/* Assumes the Icon Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_icon_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_track_segments_object(const struct bt_shell *sh,
						  size_t argc, char *argv[])
{
	/* Assumes the Segment Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_track_segments_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_current_track_object(const struct bt_shell *sh,
						 size_t argc, char *argv[])
{
	/* Assumes the Current Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_track_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_next_track_object(const struct bt_shell *sh,
					      size_t argc, char *argv[])
{
	/* Assumes the Next Track Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_next_track_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_parent_group_object(const struct bt_shell *sh,
						size_t argc, char *argv[])
{
	/* Assumes the Parent Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_parent_group_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}

static int cmd_mcc_otc_read_current_group_object(const struct bt_shell *sh,
						 size_t argc, char *argv[])
{
	/* Assumes the Current Group Object has already been selected by ID */

	int result;

	result = bt_mcc_otc_read_current_group_object(default_conn);
	if (result) {
		bt_shell_error("Fail: %d", result);
	}
	return result;
}
#endif /* CONFIG_BT_MCC_OTS */

static int cmd_mcc(const struct bt_shell *sh, size_t argc, char **argv)
{
	bt_shell_error("%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

BT_SHELL_SUBCMD_SET_CREATE(mcc_cmds,
	BT_SHELL_CMD_ARG(init, NULL, "Initialize client",
		      cmd_mcc_init, 1, 0),
	BT_SHELL_CMD_ARG(discover_mcs, NULL,
		      "Discover Media Control Service [subscribe]",
		      cmd_mcc_discover_mcs, 1, 1),
	BT_SHELL_CMD_ARG(read_player_name, NULL, "Read Media Player Name",
		      cmd_mcc_read_player_name, 1, 0),
#ifdef CONFIG_BT_MCC_OTS
	BT_SHELL_CMD_ARG(read_icon_obj_id, NULL, "Read Icon Object ID",
		      cmd_mcc_read_icon_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL)
	BT_SHELL_CMD_ARG(read_icon_url, NULL, "Read Icon URL",
		      cmd_mcc_read_icon_url, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_PLAYER_ICON_URL) */
#if defined(CONFIG_BT_MCC_READ_TRACK_TITLE)
	BT_SHELL_CMD_ARG(read_track_title, NULL, "Read Track Title",
		      cmd_mcc_read_track_title, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_TITLE) */
#if defined(CONFIG_BT_MCC_READ_TRACK_DURATION)
	BT_SHELL_CMD_ARG(read_track_duration, NULL, "Read Track Duration",
		      cmd_mcc_read_track_duration, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_DURATION) */
#if defined(CONFIG_BT_MCC_READ_TRACK_POSITION)
	BT_SHELL_CMD_ARG(read_track_position, NULL, "Read Track Position",
		      cmd_mcc_read_track_position, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_SET_TRACK_POSITION)
	BT_SHELL_CMD_ARG(set_track_position, NULL, "Set Track position <position>",
		      cmd_mcc_set_track_position, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_TRACK_POSITION) */
#if defined(CONFIG_BT_MCC_READ_PLAYBACK_SPEED)
	BT_SHELL_CMD_ARG(read_playback_speed, NULL, "Read Playback Speed",
		      cmd_mcc_read_playback_speed, 1, 0),
#endif /* defined (CONFIG_BT_MCC_READ_PLAYBACK_SPEED */
#if defined(CONFIG_BT_MCC_SET_PLAYBACK_SPEED)
	BT_SHELL_CMD_ARG(set_playback_speed, NULL, "Set Playback Speed <speed>",
		      cmd_mcc_set_playback_speed, 2, 0),
#endif /* defined (CONFIG_BT_MCC_SET_PLAYBACK_SPEED) */
#if defined(CONFIG_BT_MCC_READ_SEEKING_SPEED)
	BT_SHELL_CMD_ARG(read_seeking_speed, NULL, "Read Seeking Speed",
		      cmd_mcc_read_seeking_speed, 1, 0),
#endif /* defined (CONFIG_BT_MCC_READ_SEEKING_SPEED) */
#ifdef CONFIG_BT_MCC_OTS
	BT_SHELL_CMD_ARG(read_track_segments_obj_id, NULL,
		      "Read Track Segments Object ID",
		      cmd_mcc_read_track_segments_obj_id, 1, 0),
	BT_SHELL_CMD_ARG(read_current_track_obj_id, NULL,
		      "Read Current Track Object ID",
		      cmd_mcc_read_current_track_obj_id, 1, 0),
	BT_SHELL_CMD_ARG(set_current_track_obj_id, NULL,
		      "Set Current Track Object ID <id: 48 bits or less>",
		      cmd_mcc_set_current_track_obj_id, 2, 0),
	BT_SHELL_CMD_ARG(read_next_track_obj_id, NULL,
		      "Read Next Track Object ID",
		      cmd_mcc_read_next_track_obj_id, 1, 0),
	BT_SHELL_CMD_ARG(set_next_track_obj_id, NULL,
		      "Set Next Track Object ID <id: 48 bits or less>",
		      cmd_mcc_set_next_track_obj_id, 2, 0),
	BT_SHELL_CMD_ARG(read_current_group_obj_id, NULL,
		      "Read Current Group Object ID",
		      cmd_mcc_read_current_group_obj_id, 1, 0),
	BT_SHELL_CMD_ARG(read_parent_group_obj_id, NULL,
		      "Read Parent Group Object ID",
		      cmd_mcc_read_parent_group_obj_id, 1, 0),
	BT_SHELL_CMD_ARG(set_current_group_obj_id, NULL,
		      "Set Current Group Object ID <id: 48 bits or less>",
		      cmd_mcc_set_current_group_obj_id, 2, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER)
	BT_SHELL_CMD_ARG(read_playing_order, NULL, "Read Playing Order",
		      cmd_mcc_read_playing_order, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_SET_PLAYING_ORDER)
	BT_SHELL_CMD_ARG(set_playing_order, NULL, "Set Playing Order <order>",
		      cmd_mcc_set_playing_order, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_PLAYING_ORDER) */
#if defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED)
	BT_SHELL_CMD_ARG(read_playing_orders_supported, NULL,
		     "Read Playing Orders Supported",
		      cmd_mcc_read_playing_orders_supported, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_PLAYING_ORDER_SUPPORTED) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_STATE)
	BT_SHELL_CMD_ARG(read_media_state, NULL, "Read Media State",
		      cmd_mcc_read_media_state, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_STATE) */
#if defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT)
	BT_SHELL_CMD_ARG(play, NULL, "Send the play command", cmd_mcc_play, 1, 0),
	BT_SHELL_CMD_ARG(pause, NULL, "Send the pause command",
		      cmd_mcc_pause, 1, 0),
	BT_SHELL_CMD_ARG(fast_rewind, NULL, "Send the fast rewind command",
		      cmd_mcc_fast_rewind, 1, 0),
	BT_SHELL_CMD_ARG(fast_forward, NULL, "Send the fast forward command",
		      cmd_mcc_fast_forward, 1, 0),
	BT_SHELL_CMD_ARG(stop, NULL, "Send the stop command", cmd_mcc_stop, 1, 0),
	BT_SHELL_CMD_ARG(move_relative, NULL,
		      "Send the move relative command <int32_t: offset>",
		      cmd_mcc_move_relative, 2, 0),
	BT_SHELL_CMD_ARG(prev_segment, NULL, "Send the prev segment command",
		      cmd_mcc_prev_segment, 1, 0),
	BT_SHELL_CMD_ARG(next_segment, NULL, "Send the next segment command",
		      cmd_mcc_next_segment, 1, 0),
	BT_SHELL_CMD_ARG(first_segment, NULL, "Send the first segment command",
		      cmd_mcc_first_segment, 1, 0),
	BT_SHELL_CMD_ARG(last_segment, NULL, "Send the last segment command",
		      cmd_mcc_last_segment, 1, 0),
	BT_SHELL_CMD_ARG(goto_segment, NULL,
		      "Send the goto segment command <int32_t: segment>",
		      cmd_mcc_goto_segment, 2, 0),
	BT_SHELL_CMD_ARG(prev_track, NULL, "Send the prev track command",
		      cmd_mcc_prev_track, 1, 0),
	BT_SHELL_CMD_ARG(next_track, NULL, "Send the next track command",
		      cmd_mcc_next_track, 1, 0),
	BT_SHELL_CMD_ARG(first_track, NULL, "Send the first track command",
		      cmd_mcc_first_track, 1, 0),
	BT_SHELL_CMD_ARG(last_track, NULL, "Send the last track command",
		      cmd_mcc_last_track, 1, 0),
	BT_SHELL_CMD_ARG(goto_track, NULL,
		      "Send the goto track command  <int32_t: track>",
		      cmd_mcc_goto_track, 2, 0),
	BT_SHELL_CMD_ARG(prev_group, NULL, "Send the prev group command",
		      cmd_mcc_prev_group, 1, 0),
	BT_SHELL_CMD_ARG(next_group, NULL, "Send the next group command",
		      cmd_mcc_next_group, 1, 0),
	BT_SHELL_CMD_ARG(first_group, NULL, "Send the first group command",
		      cmd_mcc_first_group, 1, 0),
	BT_SHELL_CMD_ARG(last_group, NULL, "Send the last group command",
		      cmd_mcc_last_group, 1, 0),
	BT_SHELL_CMD_ARG(goto_group, NULL,
		      "Send the goto group command <int32_t: group>",
		      cmd_mcc_goto_group, 2, 0),
#endif /* defined(CONFIG_BT_MCC_SET_MEDIA_CONTROL_POINT) */
#if defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED)
	BT_SHELL_CMD_ARG(read_opcodes_supported, NULL, "Send the Read Opcodes Supported",
		      cmd_mcc_read_opcodes_supported, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_MEDIA_CONTROL_POINT_OPCODES_SUPPORTED) */
#ifdef CONFIG_BT_MCC_OTS
	BT_SHELL_CMD_ARG(send_search_raw, NULL, "Send search <search control item sequence>",
		      cmd_mcc_send_search_raw, 2, 0),
	BT_SHELL_CMD_ARG(send_search_scp_ioptest, NULL,
		      "Send search - IOP test round as input <round number>",
		      cmd_mcc_send_search_ioptest, 2, 0),
#if defined(CONFIG_BT_MCC_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
	BT_SHELL_CMD_ARG(test_send_search_iop_invalid_type, NULL,
		      "Send search - IOP test, invalid type value (test)",
		      cmd_mcc_test_send_search_iop_invalid_type, 1, 0),
	BT_SHELL_CMD_ARG(test_send_Search_invalid_sci_len, NULL,
		      "Send search - invalid sci length (test)",
		      cmd_mcc_test_send_search_invalid_sci_len, 1, 0),
#endif /* CONFIG_BT_MCC_LOG_LEVEL_DBG && CONFIG_BT_TESTING */
	BT_SHELL_CMD_ARG(read_search_results_obj_id, NULL,
		      "Read Search Results Object ID",
		      cmd_mcc_read_search_results_obj_id, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
#if defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID)
	BT_SHELL_CMD_ARG(read_content_control_id, NULL, "Read Content Control ID",
		      cmd_mcc_read_content_control_id, 1, 0),
#endif /* defined(CONFIG_BT_MCC_READ_CONTENT_CONTROL_ID) */
#ifdef CONFIG_BT_MCC_OTS
	BT_SHELL_CMD_ARG(ots_read_features, NULL, "Read OTC Features",
		      cmd_mcc_otc_read_features, 1, 0),
	BT_SHELL_CMD_ARG(ots_oacp_read, NULL, "Read current object",
		      cmd_mcc_otc_read, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_metadata, NULL, "Read current object's metadata",
		      cmd_mcc_otc_read_metadata, 1, 0),
	BT_SHELL_CMD_ARG(ots_select, NULL, "Select an object by its ID <ID>",
		      cmd_mcc_otc_select, 2, 0),
	BT_SHELL_CMD_ARG(ots_read_icon_object, NULL, "Read Icon Object",
		      cmd_mcc_otc_read_icon_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_track_segments_object, NULL,
		      "Read Track Segments Object",
		      cmd_mcc_otc_read_track_segments_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_current_track_object, NULL,
		      "Read Current Track Object",
		      cmd_mcc_otc_read_current_track_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_next_track_object, NULL,
		      "Read Next Track Object",
		      cmd_mcc_otc_read_next_track_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_parent_group_object, NULL,
		      "Read Parent Group Object",
		      cmd_mcc_otc_read_parent_group_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_read_current_group_object, NULL,
		      "Read Current Group Object",
		      cmd_mcc_otc_read_current_group_object, 1, 0),
	BT_SHELL_CMD_ARG(ots_select_first, NULL, "Select first object",
		      cmd_mcc_otc_select_first, 1, 0),
	BT_SHELL_CMD_ARG(ots_select_last, NULL, "Select last object",
		      cmd_mcc_otc_select_last, 1, 0),
	BT_SHELL_CMD_ARG(ots_select_next, NULL, "Select next object",
		      cmd_mcc_otc_select_next, 1, 0),
	BT_SHELL_CMD_ARG(ots_select_previous, NULL, "Select previous object",
		      cmd_mcc_otc_select_prev, 1, 0),
#endif /* CONFIG_BT_MCC_OTS */
	BT_SHELL_SUBCMD_SET_END
);

BT_SHELL_CMD_ARG_DEFINE(mcc, &mcc_cmds, "MCC commands",
		       cmd_mcc, 1, 1);

int bt_shell_cmd_mcc_register(struct bt_shell *sh)
{
	return bt_shell_cmd_register(sh, &mcc);
}
