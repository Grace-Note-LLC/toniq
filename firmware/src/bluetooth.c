#include "bluetooth.h"

static uint16_t adv_number = 1;
static uint8_t adv_number_le[2] = {1, 0};

static ssize_t adv_number_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             void *buf, uint16_t len, uint16_t offset);

// GATT service definition
BT_GATT_SERVICE_DEFINE(water_ai_service,
    BT_GATT_PRIMARY_SERVICE(
        BT_UUID_DECLARE_128(WATER_AI_SERVICE_UUID)
    ),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2A56), BT_GATT_CHRC_READ,
                         BT_GATT_PERM_READ, adv_number_read, NULL, NULL),
);

// Callback for handling characteristic reads
static ssize_t adv_number_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             void *buf, uint16_t len, uint16_t offset)
{
    uint8_t out_buffer[2];
    out_buffer[0] = adv_number & 0xFF;
    out_buffer[1] = (adv_number >> 8) & 0xFF;
    
    // printk("GATT: Read adv_number: %d\n", adv_number);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, out_buffer, sizeof(out_buffer));
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
    } else {
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, 0x8A, 0x05), // Manually set bytes for `BT_APPEARANCE_LIGHT_FIXTURES_UNDERWATER`
    { .type = BT_DATA_MANUFACTURER_DATA,
      .data = adv_number_le,
      .data_len = sizeof(adv_number_le) },
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, WATER_AI_SERVICE_UUID)
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Function to update advertised number
void bt_set_adv_number(uint16_t number)
{
    if (number > 2000) {
        printk("Invalid adv number %d; must be 0-2000\n", number);
        return;
    }
    adv_number = number;
    adv_number_le[0] = number & 0xFF;
    adv_number_le[1] = (number >> 8) & 0xFF;

    // Update ongoing advertising data
    bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

void bt_ready(int err)
{
    char addr_s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_t addr = {0};
    size_t count = 1;

    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
              sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    bt_id_get(&addr, &count);
    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

    printk("Beacon started, advertising as %s\n", addr_s);
}
