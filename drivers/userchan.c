/* userchan.c - HCI User Channel based Bluetooth driver */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Victor Chavez
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <base/byteorder.h>

#include "hci_sock.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth.h>

struct uc_data {
	int fd;
	bt_hci_recv_t recv;
	bool ready;
};

#define UC_THREAD_STACK_SIZE 2048
static os_thread_t rx_thread_data;

static unsigned short bt_dev_index;

#define TCP_ADDR_BUFF_SIZE  16
#define UNIX_ADDR_BUFF_SIZE 4096
enum hci_connection_type {
	HCI_USERCHAN,
	HCI_TCP,
	HCI_UNIX,
};
static enum hci_connection_type conn_type = HCI_USERCHAN;
static char ip_addr[TCP_ADDR_BUFF_SIZE];
static unsigned int port;
static char socket_path[UNIX_ADDR_BUFF_SIZE];

static bool is_hci_event_discardable(const struct bt_hci_evt_hdr *evt)
{
	switch (evt->evt) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		const struct bt_hci_evt_le_meta_event *meta_evt = (const void *)evt->data;

		switch (meta_evt->subevent) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
#if defined(CONFIG_BT_EXT_ADV)
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT: {
			const struct bt_hci_evt_le_ext_advertising_report *ext_adv =
				(const void *)meta_evt->data;

			return (ext_adv->num_reports == 1) &&
			       ((ext_adv->adv_info[0].evt_type & BT_HCI_LE_ADV_EVT_TYPE_LEGACY) !=
				0);
		}
#endif
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct bt_buf *get_rx_evt(const uint8_t *data)
{
	const struct bt_hci_evt_hdr *evt = (const void *)data;
	const bool discardable = is_hci_event_discardable(evt);
	const os_timeout_t timeout = discardable ? OS_TIMEOUT_NO_WAIT : OS_SECONDS(1);
	struct bt_buf *buf;

	do {
		buf = bt_buf_get_evt(evt->evt, discardable, timeout);
		if (buf == NULL) {
			if (discardable) {
				LOG_DBG("Discardable buffer pool full, ignoring event");
				return buf;
			}
			LOG_WRN("Couldn't allocate a buffer after waiting 1 second.");
		}
	} while (!buf);

	return buf;
}

static struct bt_buf *get_rx_acl(const uint8_t *data)
{
	struct bt_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, OS_TIMEOUT_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("No available ACL buffers!");
	}

	return buf;
}

static struct bt_buf *get_rx_iso(const uint8_t *data)
{
	struct bt_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, OS_TIMEOUT_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("No available ISO buffers!");
	}

	return buf;
}

static struct bt_buf *get_rx(const uint8_t *buf)
{
	uint8_t hci_h4_type = buf[0];

	switch (hci_h4_type) {
	case BT_HCI_H4_EVT:
		return get_rx_evt(&buf[1]);
	case BT_HCI_H4_ACL:
		return get_rx_acl(&buf[1]);
	case BT_HCI_H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return get_rx_iso(&buf[1]);
		}
		__fallthrough;
	default:
		LOG_ERR("Unknown packet type: %u", hci_h4_type);
	}

	return NULL;
}

/**
 * @brief Decode the length of an HCI H4 packet and check it's complete
 * @details Decodes packet length according to Bluetooth spec v5.4 Vol 4 Part E
 * @param buf	Pointer to a HCI packet buffer
 * @param buf_len	Bytes available in the buffer
 * @return Length of the complete HCI packet in bytes, -1 if cannot find an HCI
 *         packet, 0 if more data required.
 */
static int32_t hci_packet_complete(const uint8_t *buf, uint16_t buf_len)
{
	uint16_t payload_len = 0;
	const uint8_t type = buf[0];
	uint8_t header_len = sizeof(type);
	const uint8_t *hdr = &buf[sizeof(type)];

	switch (type) {
	case BT_HCI_H4_CMD: {
		if (buf_len < header_len + BT_HCI_CMD_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_cmd_hdr *cmd = (const struct bt_hci_cmd_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = cmd->param_len;
		header_len += BT_HCI_CMD_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ACL: {
		if (buf_len < header_len + BT_HCI_ACL_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_acl_hdr *acl = (const struct bt_hci_acl_hdr *)hdr;

		/* Data Total Length */
		payload_len = sys_le16_to_cpu(acl->len);
		header_len += BT_HCI_ACL_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_SCO: {
		if (buf_len < header_len + BT_HCI_SCO_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_sco_hdr *sco = (const struct bt_hci_sco_hdr *)hdr;

		/* Data_Total_Length */
		payload_len = sco->len;
		header_len += BT_HCI_SCO_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_EVT: {
		if (buf_len < header_len + BT_HCI_EVT_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_evt_hdr *evt = (const struct bt_hci_evt_hdr *)hdr;

		/* Parameter Total Length */
		payload_len = evt->len;
		header_len += BT_HCI_EVT_HDR_SIZE;
		break;
	}
	case BT_HCI_H4_ISO: {
		if (buf_len < header_len + BT_HCI_ISO_HDR_SIZE) {
			return 0;
		}
		const struct bt_hci_iso_hdr *iso = (const struct bt_hci_iso_hdr *)hdr;

		/* ISO_Data_Load_Length parameter */
		payload_len = bt_iso_hdr_len(sys_le16_to_cpu(iso->len));
		header_len += BT_HCI_ISO_HDR_SIZE;
		break;
	}
	/* If no valid packet type found */
	default:
		LOG_WRN("Unknown packet type 0x%02x", type);
		return -1;
	}

	/* Request more data */
	if (buf_len < header_len + payload_len) {
		return 0;
	}

	return (int32_t)header_len + payload_len;
}

static void rx_thread(void *p1)
{
	const struct bt_hci_transport *transport = p1;
	struct uc_data *uc = transport->user_data;

	LOG_DBG("started");

	long frame_size = 0;

	while (1) {
		static uint8_t frame[1021];
		struct bt_buf *buf;
		size_t buf_tailroom;
		size_t buf_add_len;
		long len;
		const uint8_t *frame_start = frame;

		if (!hci_sock_rx_ready(uc->fd)) {
			os_sleep_ms(1);
			continue;
		}

		if (frame_size >= sizeof(frame)) {
			LOG_ERR("HCI Packet is too big for frame (%zu "
				"bytes). Dropping data",
				sizeof(frame));
			frame_size = 0; /* Drop buffer */
		}

		LOG_DBG("calling read()");

		len = read(uc->fd, frame + frame_size, sizeof(frame) - frame_size);
		if (len < 0) {
			if (errno == EINTR) {
				os_thread_yield();
				continue;
			}

			LOG_ERR("Reading socket failed, errno %d", errno);
			(void)close(uc->fd);
			uc->fd = -1;
			return;
		}

		frame_size += len;

		while (frame_size > 0) {
			const uint8_t *buf_add;
			const uint8_t packet_type = frame_start[0];
			const int32_t decoded_len = hci_packet_complete(frame_start, frame_size);

			if (decoded_len == -1) {
				LOG_ERR("HCI Packet type is invalid, length could not be decoded");
				frame_size = 0; /* Drop buffer */
				break;
			}

			if (decoded_len == 0) {
				if ((frame_start != frame) && (frame_size < sizeof(frame))) {
					memmove(frame, frame_start, frame_size);
				}
				/* Read more */
				break;
			}

			buf_add = frame_start + sizeof(packet_type);
			buf_add_len = decoded_len - sizeof(packet_type);

			buf = get_rx(frame_start);

			frame_size -= decoded_len;
			frame_start += decoded_len;

			if (!buf) {
				continue;
			}

			buf_tailroom = bt_buf_tailroom(buf);
			if (buf_tailroom < buf_add_len) {
				LOG_ERR("Not enough space in buffer %zu/%zu", buf_add_len,
					buf_tailroom);
				bt_buf_unref(buf);
				continue;
			}

			bt_buf_add_mem(buf, buf_add, buf_add_len);

			LOG_DBG("Calling bt_recv(%p)", buf);

			uc->recv(transport, buf);
		}

		os_thread_yield();
	}
}

static int uc_send(const struct bt_hci_transport *transport, struct bt_buf *buf)
{
	struct uc_data *uc = transport->user_data;

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

	if (uc->fd < 0) {
		LOG_ERR("User channel not open");
		return -EIO;
	}

	if (write(uc->fd, buf->data, buf->len) < 0) {
		return -errno;
	}

	bt_buf_unref(buf);
	return 0;
}

static int uc_open(const struct bt_hci_transport *transport, bt_hci_recv_t recv)
{
	struct uc_data *uc = transport->user_data;

	switch (conn_type) {
	case HCI_USERCHAN:
		LOG_DBG("hci%d", bt_dev_index);
		uc->fd = hci_sock_socket_open(bt_dev_index);
		break;
	case HCI_TCP:
		LOG_DBG("hci %s:%d", ip_addr, port);
		uc->fd = hci_sock_net_connect(ip_addr, port);
		break;
	case HCI_UNIX:
		LOG_DBG("hci socket %s", socket_path);
		uc->fd = hci_sock_unix_connect(socket_path);
		break;
	}
	if (uc->fd < 0) {
		return uc->fd;
	}

	uc->recv = recv;

	LOG_DBG("User Channel opened as fd %d", uc->fd);

	os_thread_create(&rx_thread_data, rx_thread, (void *)transport, "user_chan",
			 OS_PRIORITY(CONFIG_BT_DRIVER_RX_HIGH_PRIO), UC_THREAD_STACK_SIZE);

	LOG_DBG("returning");

	return 0;
}

static int uc_close(const struct bt_hci_transport *transport)
{
	struct uc_data *uc = transport->user_data;
	int rc;

	if (uc->fd < 0) {
		return -ENETDOWN;
	}

	rc = close(uc->fd);
	if (rc < 0) {
		return rc;
	}

	uc->fd = -1;

	return 0;
}

static const struct bt_hci_driver_api uc_drv_api = {
	.open = uc_open,
	.close = uc_close,
	.send = uc_send,
};

static bool uc_is_ready(const struct bt_hci_transport *transport)
{
	struct uc_data *uc = transport->user_data;

	return uc->ready;
}

static struct uc_data _uc_data = {
	.fd = -1,
	.ready = false,
};

const struct bt_hci_transport uc_transport = {
	.name = "userchan",
	.bus = BT_HCI_BUS_VIRTIO,
	.api = &uc_drv_api,
	.user_data = &_uc_data,
	.is_ready = uc_is_ready,
};

static int uc_init(void)
{
	struct uc_data *uc = &_uc_data;

	printf("uc_init\n");
	bt_hci_transport_register(&uc_transport);

	uc->fd = -1;
	uc->ready = true;

	return 0;
}

int bt_driver_userchan_init(void)
{
	return uc_init();
}

STACK_INIT(uc_init, STACK_BASE_INIT, 0);
