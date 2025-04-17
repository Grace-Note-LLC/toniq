#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>

#define DEVICE_NAME "water.ai"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_BYTES_LIST_LE16(_v)         \
        (((_v) >>  0) & 0xFFU),     \
        (((_v) >>  8) & 0xFFU)      \

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(0x05C1))
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

void bt_ready(int err);

#endif // BLUETOOTH_H
