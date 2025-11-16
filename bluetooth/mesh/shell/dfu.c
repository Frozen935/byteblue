/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dfu.h"

#include <stdlib.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/shell.h>

#include "common/bt_shell_private.h"
#include "utils.h"
#include "blob.h"
#include "../dfu_slot.h"

/***************************************************************************************************
 * Implementation of models' instances
 **************************************************************************************************/

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)

static void dfu_cli_ended(struct bt_mesh_dfu_cli *cli,
			  enum bt_mesh_dfu_status reason)
{
	bt_shell_print("DFU ended: %u", reason);
}

static void dfu_cli_applied(struct bt_mesh_dfu_cli *cli)
{
	bt_shell_print("DFU applied.");
}

static void dfu_cli_lost_target(struct bt_mesh_dfu_cli *cli,
				struct bt_mesh_dfu_target *target)
{
	bt_shell_print("DFU target lost: 0x%04x", target->blob.addr);
}

static void dfu_cli_confirmed(struct bt_mesh_dfu_cli *cli)
{
	bt_shell_print("DFU confirmed");
}

const struct bt_mesh_dfu_cli_cb dfu_cli_cb = {
	.ended = dfu_cli_ended,
	.applied = dfu_cli_applied,
	.lost_target = dfu_cli_lost_target,
	.confirmed = dfu_cli_confirmed,
};

struct bt_mesh_dfu_cli bt_mesh_shell_dfu_cli = BT_MESH_DFU_CLI_INIT(&dfu_cli_cb);

#endif /* CONFIG_BT_MESH_SHELL_DFU_CLI */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)

struct shell_dfu_fwid {
	uint8_t type;
	struct mcuboot_img_sem_ver ver;
};

static struct bt_mesh_dfu_img dfu_imgs[] = { {
	.fwid = &((struct shell_dfu_fwid){ 0x01, { 1, 0, 0, 0 } }),
	.fwid_len = sizeof(struct shell_dfu_fwid),
} };

static int dfu_meta_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct bt_buf_simple *metadata,
			  enum bt_mesh_dfu_effect *effect)
{
	return 0;
}

static int dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct bt_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	bt_shell_print("DFU setup");

	*io = bt_mesh_shell_blob_io;

	return 0;
}

static void dfu_end(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img, bool success)
{
	if (!success) {
		bt_shell_print("DFU failed");
		return;
	}

	if (!bt_mesh_shell_blob_valid) {
		bt_mesh_dfu_srv_rejected(srv);
		return;
	}

	bt_mesh_dfu_srv_verified(srv);
}

static int dfu_apply(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img)
{
	if (!bt_mesh_shell_blob_valid) {
		return -EINVAL;
	}

	bt_shell_print("Applying DFU transfer...");

	return 0;
}

static const struct bt_mesh_dfu_srv_cb dfu_handlers = {
	.check = dfu_meta_check,
	.start = dfu_start,
	.end = dfu_end,
	.apply = dfu_apply,
};

struct bt_mesh_dfu_srv bt_mesh_shell_dfu_srv =
	BT_MESH_DFU_SRV_INIT(&dfu_handlers, dfu_imgs, ARRAY_SIZE(dfu_imgs));

#endif /* CONFIG_BT_MESH_SHELL_DFU_SRV */

void bt_mesh_shell_dfu_cmds_init(void)
{
#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV) && defined(CONFIG_BOOTLOADER_MCUBOOT)
	struct mcuboot_img_header img_header;

	int err = boot_read_bank_header(FIXED_PARTITION_ID(slot0_partition),
					&img_header, sizeof(img_header));
	if (!err) {
		struct shell_dfu_fwid *fwid =
			(struct shell_dfu_fwid *)dfu_imgs[0].fwid;

		fwid->ver = img_header.h.v1.sem_ver;

		boot_write_img_confirmed();
	}
#endif
}

/***************************************************************************************************
 * Shell Commands
 **************************************************************************************************/

#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)

BT_BUF_SIMPLE_DEFINE_STATIC(dfu_comp_data, BT_MESH_TX_SDU_MAX);

static int cmd_dfu_comp_clear(const struct bt_shell *sh, size_t argc, char *argv[])
{
	bt_buf_simple_reset(&dfu_comp_data);
	return 0;
}

static int cmd_dfu_comp_add(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_buf_simple_state state;
	int err = 0;

	if (argc < 6) {
		return -EINVAL;
	}

	if (bt_buf_simple_tailroom(&dfu_comp_data) < 10) {
		bt_shell_print("Buffer is too small: %u",
			    bt_buf_simple_tailroom(&dfu_comp_data));
		return -EMSGSIZE;
	}

	bt_buf_simple_save(&dfu_comp_data, &state);

	for (size_t i = 1; i <= 5; i++) {
		bt_buf_simple_add_le16(&dfu_comp_data, bt_shell_strtoul(argv[i], 0, &err));
	}

	if (err) {
		bt_buf_simple_restore(&dfu_comp_data, &state);
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	return 0;
}

static int cmd_dfu_comp_elem_add(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint8_t sig_model_count;
	uint8_t vnd_model_count;
	struct bt_buf_simple_state state;
	int err = 0;

	if (argc < 5) {
		return -EINVAL;
	}

	bt_buf_simple_save(&dfu_comp_data, &state);

	sig_model_count = bt_shell_strtoul(argv[2], 0, &err);
	vnd_model_count = bt_shell_strtoul(argv[3], 0, &err);

	if (argc < 4 + sig_model_count + vnd_model_count * 2) {
		return -EINVAL;
	}

	if (bt_buf_simple_tailroom(&dfu_comp_data) < 4 + sig_model_count * 2 +
	    vnd_model_count * 4) {
		bt_shell_print("Buffer is too small: %u",
			    bt_buf_simple_tailroom(&dfu_comp_data));
		return -EMSGSIZE;
	}

	bt_buf_simple_add_le16(&dfu_comp_data, bt_shell_strtoul(argv[1], 0, &err));
	bt_buf_simple_add_u8(&dfu_comp_data, sig_model_count);
	bt_buf_simple_add_u8(&dfu_comp_data, vnd_model_count);

	for (size_t i = 0; i < sig_model_count; i++) {
		bt_buf_simple_add_le16(&dfu_comp_data, bt_shell_strtoul(argv[4 + i], 0, &err));
	}

	for (size_t i = 0; i < vnd_model_count; i++) {
		size_t arg_i = 4 + sig_model_count + i * 2;

		bt_buf_simple_add_le16(&dfu_comp_data, bt_shell_strtoul(argv[arg_i], 0, &err));
		bt_buf_simple_add_le16(&dfu_comp_data, bt_shell_strtoul(argv[arg_i + 1], 0, &err));
	}

	if (err) {
		bt_buf_simple_restore(&dfu_comp_data, &state);
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	return 0;
}

static int cmd_dfu_comp_hash_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint8_t key[16] = {};
	uint32_t hash;
	int err;

	if (dfu_comp_data.len < 14) {
		bt_shell_print("Composition data is not set");
		return -EINVAL;
	}

	if (argc > 1) {
		hex2bin(argv[1], strlen(argv[1]), key, sizeof(key));
	}

	bt_shell_print("Composition data to be hashed:");
	bt_shell_print("\tCID: 0x%04x", sys_get_le16(&dfu_comp_data.data[0]));
	bt_shell_print("\tPID: 0x%04x", sys_get_le16(&dfu_comp_data.data[2]));
	bt_shell_print("\tVID: 0x%04x", sys_get_le16(&dfu_comp_data.data[4]));
	bt_shell_print("\tCPRL: %u", sys_get_le16(&dfu_comp_data.data[6]));
	bt_shell_print("\tFeatures: 0x%x", sys_get_le16(&dfu_comp_data.data[8]));

	for (size_t i = 10; i < dfu_comp_data.len - 4;) {
		uint8_t sig_model_count = dfu_comp_data.data[i + 2];
		uint8_t vnd_model_count = dfu_comp_data.data[i + 3];

		bt_shell_print("\tElem: %u", sys_get_le16(&dfu_comp_data.data[i]));
		bt_shell_print("\t\tNumS: %u", sig_model_count);
		bt_shell_print("\t\tNumV: %u", vnd_model_count);

		for (size_t j = 0; j < sig_model_count; j++) {
			bt_shell_print("\t\tSIG Model ID: 0x%04x",
				    sys_get_le16(&dfu_comp_data.data[i + 4 + j * 2]));
		}

		for (size_t j = 0; j < vnd_model_count; j++) {
			size_t arg_i = i + 4 + sig_model_count * 2 + j * 4;

			bt_shell_print("\t\tVnd Company ID: 0x%04x, Model ID: 0x%04x",
				    sys_get_le16(&dfu_comp_data.data[arg_i]),
				    sys_get_le16(&dfu_comp_data.data[arg_i + 2]));
		}

		i += 4 + sig_model_count * 2 + vnd_model_count * 4;
	}

	err = bt_mesh_dfu_metadata_comp_hash_get(&dfu_comp_data, key, &hash);
	if (err) {
		bt_shell_print("Failed to compute composition data hash: %d\n", err);
		return err;
	}

	bt_shell_print("Composition data hash: 0x%04x", hash);

	return 0;
}

static int cmd_dfu_metadata_encode(const struct bt_shell *sh, size_t argc, char *argv[])
{
	char md_str[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	uint8_t user_data[CONFIG_BT_MESH_DFU_METADATA_MAXLEN - 18];
	struct bt_mesh_dfu_metadata md;
	size_t len;
	int err = 0;

	BT_BUF_SIMPLE_DEFINE(buf, CONFIG_BT_MESH_DFU_METADATA_MAXLEN);

	if (argc < 9) {
		return -EINVAL;
	}

	md.fw_ver.major = bt_shell_strtoul(argv[1], 0, &err);
	md.fw_ver.minor = bt_shell_strtoul(argv[2], 0, &err);
	md.fw_ver.revision = bt_shell_strtoul(argv[3], 0, &err);
	md.fw_ver.build_num = bt_shell_strtoul(argv[4], 0, &err);
	md.fw_size = bt_shell_strtoul(argv[5], 0, &err);
	md.fw_core_type = bt_shell_strtoul(argv[6], 0, &err);
	md.comp_hash = bt_shell_strtoul(argv[7], 0, &err);
	md.elems = bt_shell_strtoul(argv[8], 0, &err);

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	if (argc > 9) {
		if (sizeof(user_data) < strlen(argv[9]) / 2) {
			bt_shell_print("User data is too big.");
			return -EINVAL;
		}

		md.user_data_len = hex2bin(argv[9], strlen(argv[9]), user_data, sizeof(user_data));
		md.user_data = user_data;
	} else {
		md.user_data_len = 0;
	}

	bt_shell_print("Metadata to be encoded:");
	bt_shell_print("\tVersion: %u.%u.%u+%u", md.fw_ver.major, md.fw_ver.minor,
		    md.fw_ver.revision, md.fw_ver.build_num);
	bt_shell_print("\tSize: %u", md.fw_size);
	bt_shell_print("\tCore Type: 0x%x", md.fw_core_type);
	bt_shell_print("\tComposition data hash: 0x%x", md.comp_hash);
	bt_shell_print("\tElements: %u", md.elems);

	if (argc > 9) {
		bt_shell_print("\tUser data: %s", argv[10]);
	}

	bt_shell_print("\tUser data length: %u", md.user_data_len);

	err = bt_mesh_dfu_metadata_encode(&md, &buf);
	if (err) {
		bt_shell_print("Failed to encode metadata: %d", err);
		return err;
	}

	len = bin2hex(buf.data, buf.len, md_str, sizeof(md_str));
	md_str[len] = '\0';
	bt_shell_print("Encoded metadata: %s", md_str);

	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_METADATA */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)

static int cmd_dfu_slot_add(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_slot *slot;
	size_t size;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	size_t fwid_len = 0;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN];
	size_t metadata_len = 0;
	int err = 0;

	size = bt_shell_strtoul(argv[1], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	bt_shell_print("Adding slot (size: %u)", size);
	slot = bt_mesh_dfu_slot_reserve();

	if (!slot) {
		bt_shell_print("Failed to reserve slot.");
		return 0;
	}

	fwid_len = hex2bin(argv[2], strlen(argv[2]), fwid,
			   sizeof(fwid));
	bt_mesh_dfu_slot_fwid_set(slot, fwid, fwid_len);

	if (argc > 3) {
		metadata_len = hex2bin(argv[3], strlen(argv[3]), metadata,
				       sizeof(metadata));
	}

	bt_mesh_dfu_slot_info_set(slot, size, metadata, metadata_len);

	err = bt_mesh_dfu_slot_commit(slot);
	if (err) {
		bt_shell_print("Failed to commit slot: %d", err);
		bt_mesh_dfu_slot_release(slot);
		return err;
	}

	bt_shell_print("Slot added. Index: %u", bt_mesh_dfu_slot_img_idx_get(slot));

	return 0;
}

static int cmd_dfu_slot_del(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_dfu_slot *slot;
	uint8_t idx;
	int err = 0;

	idx = bt_shell_strtoul(argv[1], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(idx);
	if (!slot) {
		bt_shell_print("No slot at %u", idx);
		return 0;
	}

	err = bt_mesh_dfu_slot_del(slot);
	if (err) {
		bt_shell_print("Failed deleting slot %u (err: %d)", idx,
			    err);
		return 0;
	}

	bt_shell_print("Slot %u deleted.", idx);
	return 0;
}

static int cmd_dfu_slot_del_all(const struct bt_shell *sh, size_t argc, char *argv[])
{
	bt_mesh_dfu_slot_del_all();
	bt_shell_print("All slots deleted.");
	return 0;
}

static void slot_info_print(const struct bt_shell *sh, const struct bt_mesh_dfu_slot *slot,
			    const uint8_t *idx)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	char metadata[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	size_t len;

	len = bin2hex(slot->fwid, slot->fwid_len, fwid, sizeof(fwid));
	fwid[len] = '\0';
	len = bin2hex(slot->metadata, slot->metadata_len, metadata,
		      sizeof(metadata));
	metadata[len] = '\0';

	if (idx != NULL) {
		bt_shell_print("Slot %u:", *idx);
	} else {
		bt_shell_print("Slot:");
	}
	bt_shell_print("\tSize:     %u bytes", slot->size);
	bt_shell_print("\tFWID:     %s", fwid);
	bt_shell_print("\tMetadata: %s", metadata);
}

static int cmd_dfu_slot_get(const struct bt_shell *sh, size_t argc, char *argv[])
{
	const struct bt_mesh_dfu_slot *slot;
	uint8_t idx;
	int err = 0;

	idx = bt_shell_strtoul(argv[1], 0, &err);
	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(idx);
	if (!slot) {
		bt_shell_print("No slot at %u", idx);
		return 0;
	}

	slot_info_print(sh, slot, &idx);
	return 0;
}

#endif /* defined(CONFIG_BT_MESH_SHELL_DFU_SLOT) */

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)

static const struct bt_mesh_model *mod_cli;

static struct {
	struct bt_mesh_dfu_target targets[32];
	struct bt_mesh_blob_target_pull pull[32];
	size_t target_cnt;
	struct bt_mesh_blob_cli_inputs inputs;
} dfu_tx;

static void dfu_tx_prepare(void)
{
	bt_slist_init(&dfu_tx.inputs.targets);

	for (size_t i = 0; i < dfu_tx.target_cnt; i++) {
		/* Reset target context. */
		uint16_t addr = dfu_tx.targets[i].blob.addr;

		memset(&dfu_tx.targets[i].blob, 0, sizeof(struct bt_mesh_blob_target));
		memset(&dfu_tx.pull[i], 0, sizeof(struct bt_mesh_blob_target_pull));
		dfu_tx.targets[i].blob.addr = addr;
		dfu_tx.targets[i].blob.pull = &dfu_tx.pull[i];

		bt_slist_append(&dfu_tx.inputs.targets, &dfu_tx.targets[i].blob.n);
	}
}

static int cmd_dfu_target(const struct bt_shell *sh, size_t argc, char *argv[])
{
	uint8_t img_idx;
	uint16_t addr;
	int err = 0;

	addr = bt_shell_strtoul(argv[1], 0, &err);
	img_idx = bt_shell_strtoul(argv[2], 0, &err);

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	if (dfu_tx.target_cnt == ARRAY_SIZE(dfu_tx.targets)) {
		bt_shell_print("No room.");
		return 0;
	}

	for (size_t i = 0; i < dfu_tx.target_cnt; i++) {
		if (dfu_tx.targets[i].blob.addr == addr) {
			bt_shell_print("Target 0x%04x already exists", addr);
			return 0;
		}
	}

	dfu_tx.targets[dfu_tx.target_cnt].blob.addr = addr;
	dfu_tx.targets[dfu_tx.target_cnt].img_idx = img_idx;
	bt_slist_append(&dfu_tx.inputs.targets, &dfu_tx.targets[dfu_tx.target_cnt].blob.n);
	dfu_tx.target_cnt++;

	bt_shell_print("Added target 0x%04x", addr);
	return 0;
}

static int cmd_dfu_targets_reset(const struct bt_shell *sh, size_t argc, char *argv[])
{
	dfu_tx_prepare();
	return 0;
}

static int cmd_dfu_target_state(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_target_status rsp;
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	err = bt_mesh_dfu_cli_status_get((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data,
					 &ctx, &rsp);
	if (err) {
		bt_shell_print("Failed getting target status (err: %d)",
			    err);
		return 0;
	}

	bt_shell_print("Target 0x%04x:", bt_mesh_shell_target_ctx.dst);
	bt_shell_print("\tStatus:     %u", rsp.status);
	bt_shell_print("\tPhase:      %u", rsp.phase);
	if (rsp.phase != BT_MESH_DFU_PHASE_IDLE) {
		bt_shell_print("\tEffect:       %u", rsp.effect);
		bt_shell_print("\tImg Idx:      %u", rsp.img_idx);
		bt_shell_print("\tTTL:          %u", rsp.ttl);
		bt_shell_print("\tTimeout base: %u", rsp.timeout_base);
	}

	return 0;
}

static enum bt_mesh_dfu_iter dfu_img_cb(struct bt_mesh_dfu_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					uint8_t idx, uint8_t total,
					const struct bt_mesh_dfu_img *img,
					void *cb_data)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	size_t len;

	len = bin2hex(img->fwid, img->fwid_len, fwid, sizeof(fwid));
	fwid[len] = '\0';

	bt_shell_print("Image %u:", idx);
	bt_shell_print("\tFWID: %s", fwid);
	if (img->uri) {
		bt_shell_print("\tURI:  %s", img->uri);
	}

	return BT_MESH_DFU_ITER_CONTINUE;
}

static int cmd_dfu_target_imgs(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	uint8_t img_cnt = 0xff;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	if (argc == 2) {
		img_cnt = bt_shell_strtoul(argv[1], 0, &err);
		if (err) {
			bt_shell_warn("Unable to parse input string argument");
			return err;
		}
	}

	bt_shell_print("Requesting DFU images in 0x%04x", bt_mesh_shell_target_ctx.dst);

	err = bt_mesh_dfu_cli_imgs_get((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data,
				       &ctx, dfu_img_cb, NULL, img_cnt);
	if (err) {
		bt_shell_print("Request failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_target_check(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_metadata_status rsp;
	const struct bt_mesh_dfu_slot *slot;
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	uint8_t slot_idx, img_idx;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	slot_idx = bt_shell_strtoul(argv[1], 0, &err);
	img_idx = bt_shell_strtoul(argv[2], 0, &err);

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!slot) {
		bt_shell_print("No image in slot %u", slot_idx);
		return 0;
	}

	err = bt_mesh_dfu_cli_metadata_check((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data,
					     &ctx, img_idx, slot, &rsp);
	if (err) {
		bt_shell_print("Metadata check failed. err: %d", err);
		return 0;
	}

	bt_shell_print("Slot %u check for 0x%04x image %u:", slot_idx,
		    bt_mesh_shell_target_ctx.dst, img_idx);
	bt_shell_print("\tStatus: %u", rsp.status);
	bt_shell_print("\tEffect: 0x%x", rsp.effect);

	return 0;
}

static int cmd_dfu_send(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_dfu_cli_xfer_blob_params blob_params;
	struct bt_mesh_dfu_cli_xfer xfer = { 0 };
	uint8_t slot_idx;
	uint16_t group;
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	slot_idx = bt_shell_strtoul(argv[1], 0, &err);
	if (argc > 2) {
		group = bt_shell_strtoul(argv[2], 0, &err);
	} else {
		group = BT_MESH_ADDR_UNASSIGNED;
	}

	if (argc > 3) {
		xfer.mode = bt_shell_strtoul(argv[3], 0, &err);
	} else {
		xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	}

	if (argc > 5) {
		blob_params.block_size_log = bt_shell_strtoul(argv[4], 0, &err);
		blob_params.chunk_size = bt_shell_strtoul(argv[5], 0, &err);
		xfer.blob_params = &blob_params;
	} else {
		xfer.blob_params = NULL;
	}

	if (err) {
		bt_shell_warn("Unable to parse input string argument");
		return err;
	}

	if (!dfu_tx.target_cnt) {
		bt_shell_print("No targets.");
		return 0;
	}

	xfer.slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!xfer.slot) {
		bt_shell_print("No image in slot %u", slot_idx);
		return 0;
	}

	bt_shell_print("Starting DFU from slot %u (%u targets)", slot_idx,
		    dfu_tx.target_cnt);

	dfu_tx.inputs.group = group;
	dfu_tx.inputs.app_idx = bt_mesh_shell_target_ctx.app_idx;
	dfu_tx.inputs.ttl = BT_MESH_TTL_DEFAULT;

	err = bt_mesh_dfu_cli_send((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data,
				   &dfu_tx.inputs, bt_mesh_shell_blob_io, &xfer);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
		return 0;
	}
	return 0;
}

static int cmd_dfu_tx_cancel(const struct bt_shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = bt_mesh_shell_target_ctx.net_idx,
		.addr = bt_mesh_shell_target_ctx.dst,
		.app_idx = bt_mesh_shell_target_ctx.app_idx,
	};
	int err = 0;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	if (argc == 2) {
		ctx.addr = bt_shell_strtoul(argv[1], 0, &err);
		if (err) {
			bt_shell_warn("Unable to parse input string argument");
			return err;
		}

		bt_shell_print("Cancelling DFU for 0x%04x", ctx.addr);
	} else {
		bt_shell_print("Cancelling DFU");
	}

	err = bt_mesh_dfu_cli_cancel((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data,
				     (argc == 2) ? &ctx : NULL);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_apply(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	bt_shell_print("Applying DFU");

	err = bt_mesh_dfu_cli_apply((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_confirm(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	bt_shell_print("Confirming DFU");

	err = bt_mesh_dfu_cli_confirm((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_suspend(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	bt_shell_print("Suspending DFU");

	err = bt_mesh_dfu_cli_suspend((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_resume(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	bt_shell_print("Resuming DFU");

	err = bt_mesh_dfu_cli_resume((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data);
	if (err) {
		bt_shell_print("Failed (err: %d)", err);
	}

	return 0;
}

static int cmd_dfu_tx_progress(const struct bt_shell *sh, size_t argc, char *argv[])
{
	if (!mod_cli && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_CLI, &mod_cli)) {
		return -ENODEV;
	}

	bt_shell_print("DFU progress: %u %%",
		    bt_mesh_dfu_cli_progress((struct bt_mesh_dfu_cli *)mod_cli->rt->user_data));
	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_CLI */

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)

static const struct bt_mesh_model *mod_srv;

static int cmd_dfu_applied(const struct bt_shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	bt_mesh_dfu_srv_applied((struct bt_mesh_dfu_srv *)mod_srv->rt->user_data);
	return 0;
}

static int cmd_dfu_rx_cancel(const struct bt_shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	bt_mesh_dfu_srv_cancel((struct bt_mesh_dfu_srv *)mod_srv->rt->user_data);
	return 0;
}

static int cmd_dfu_rx_progress(const struct bt_shell *sh, size_t argc, char *argv[])
{
	if (!mod_srv && !bt_mesh_shell_mdl_first_get(BT_MESH_MODEL_ID_DFU_SRV, &mod_srv)) {
		return -ENODEV;
	}

	bt_shell_print("DFU progress: %u %%",
		    bt_mesh_dfu_srv_progress((struct bt_mesh_dfu_srv *)mod_srv->rt->user_data));
	return 0;
}

#endif /* CONFIG_BT_MESH_SHELL_DFU_SRV */

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(cli_instance_cmds, BT_MESH_MODEL_ID_DFU_CLI, mod_cli);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
BT_MESH_SHELL_MDL_INSTANCE_CMDS(srv_instance_cmds, BT_MESH_MODEL_ID_DFU_SRV, mod_srv);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)
BT_SHELL_SUBCMD_SET_CREATE(
	dfu_metadata_cmds,
	BT_SHELL_CMD_ARG(comp-clear, NULL, NULL, cmd_dfu_comp_clear, 1, 0),
	BT_SHELL_CMD_ARG(comp-add, NULL, "<CID> <ProductID> <VendorID> <Crpl> <Features>",
		      cmd_dfu_comp_add, 6, 0),
	BT_SHELL_CMD_ARG(comp-elem-add, NULL, "<Loc> <NumS> <NumV> "
		      "{<SigMID>|<VndCID> <VndMID>}...",
		      cmd_dfu_comp_elem_add, 5, 10),
	BT_SHELL_CMD_ARG(comp-hash-get, NULL, "[<Key>]", cmd_dfu_comp_hash_get, 1, 1),
	BT_SHELL_CMD_ARG(encode, NULL, "<Major> <Minor> <Rev> <BuildNum> <Size> "
		      "<CoreType> <Hash> <Elems> [<UserData>]",
		      cmd_dfu_metadata_encode, 9, 1),
	BT_SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)
BT_SHELL_SUBCMD_SET_CREATE(
	dfu_slot_cmds,
	BT_SHELL_CMD_ARG(add, NULL,
		      "<Size> <FwID> [<Metadata>]",
		      cmd_dfu_slot_add, 3, 1),
	BT_SHELL_CMD_ARG(del, NULL, "<SlotIdx>", cmd_dfu_slot_del, 2, 0),
	BT_SHELL_CMD_ARG(del-all, NULL, NULL, cmd_dfu_slot_del_all, 1, 0),
	BT_SHELL_CMD_ARG(get, NULL, "<SlotIdx>", cmd_dfu_slot_get, 2, 0),
	BT_SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
BT_SHELL_SUBCMD_SET_CREATE(
	dfu_cli_cmds,
	/* DFU Client Model Operations */
	BT_SHELL_CMD_ARG(target, NULL, "<Addr> <ImgIdx>", cmd_dfu_target, 3,
		      0),
	BT_SHELL_CMD_ARG(targets-reset, NULL, NULL, cmd_dfu_targets_reset, 1, 0),
	BT_SHELL_CMD_ARG(target-state, NULL, NULL, cmd_dfu_target_state, 1, 0),
	BT_SHELL_CMD_ARG(target-imgs, NULL, "[<MaxCount>]",
		      cmd_dfu_target_imgs, 1, 1),
	BT_SHELL_CMD_ARG(target-check, NULL, "<SlotIdx> <TargetImgIdx>",
		      cmd_dfu_target_check, 3, 0),
	BT_SHELL_CMD_ARG(send, NULL, "<SlotIdx>  [<Group> "
		      "[<Mode(push, pull)> [<BlockSizeLog> <ChunkSize>]]]", cmd_dfu_send, 2, 4),
	BT_SHELL_CMD_ARG(cancel, NULL, "[<Addr>]", cmd_dfu_tx_cancel, 1, 1),
	BT_SHELL_CMD_ARG(apply, NULL, NULL, cmd_dfu_apply, 0, 0),
	BT_SHELL_CMD_ARG(confirm, NULL, NULL, cmd_dfu_confirm, 0, 0),
	BT_SHELL_CMD_ARG(suspend, NULL, NULL, cmd_dfu_suspend, 0, 0),
	BT_SHELL_CMD_ARG(resume, NULL, NULL, cmd_dfu_resume, 0, 0),
	BT_SHELL_CMD_ARG(progress, NULL, NULL, cmd_dfu_tx_progress, 1, 0),
	BT_SHELL_CMD(instance, &cli_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	BT_SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
BT_SHELL_SUBCMD_SET_CREATE(
	dfu_srv_cmds,
	BT_SHELL_CMD_ARG(applied, NULL, NULL, cmd_dfu_applied, 1, 0),
	BT_SHELL_CMD_ARG(rx-cancel, NULL, NULL, cmd_dfu_rx_cancel, 1, 0),
	BT_SHELL_CMD_ARG(progress, NULL, NULL, cmd_dfu_rx_progress, 1, 0),
	BT_SHELL_CMD(instance, &srv_instance_cmds, "Instance commands", bt_mesh_shell_mdl_cmds_help),
	BT_SHELL_SUBCMD_SET_END);
#endif

BT_SHELL_SUBCMD_SET_CREATE(
	dfu_cmds,
#if defined(CONFIG_BT_MESH_SHELL_DFU_METADATA)
	BT_SHELL_CMD(metadata, &dfu_metadata_cmds, "Metadata commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_SLOT)
	BT_SHELL_CMD(slot, &dfu_slot_cmds, "Slot commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI)
	BT_SHELL_CMD(cli, &dfu_cli_cmds, "DFU Cli commands", bt_mesh_shell_mdl_cmds_help),
#endif
#if defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
	BT_SHELL_CMD(srv, &dfu_srv_cmds, "DFU Srv commands", bt_mesh_shell_mdl_cmds_help),
#endif
	BT_SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), dfu, &dfu_cmds, "DFU models commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
