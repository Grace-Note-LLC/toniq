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
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>

#define DEVICE_NAME "water.ai"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BT_BYTES_LIST_LE16(_v)      \
        (((_v) >>  0) & 0xFFU),     \
        (((_v) >>  8) & 0xFFU)      \

// Define service UUID
#define WATER_AI_SERVICE_UUID BT_UUID_128_ENCODE(0x6E536C92, 0x0184, 0x47AD, 0xA0F9, 0x54BCE6B10D1E)

// Function called when Bluetooth is ready
void bt_ready(int err);

// Function to update advertised number (0-2000)
void bt_set_adv_number(uint16_t number);

#endif // BLUETOOTH_H
