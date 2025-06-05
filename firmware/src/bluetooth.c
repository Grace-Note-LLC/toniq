#include "bluetooth.h"

static uint16_t adv_number = 1;
static uint8_t adv_number_le[2] = {1, 0};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_APPEARANCE_LIGHT_FIXTURES_UNDERWATER),
	{ .type = BT_DATA_MANUFACTURER_DATA,
	  .data = adv_number_le,
	  .data_len = sizeof(adv_number_le) }
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

	/* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
              sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}


	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("Beacon started, advertising as %s\n", addr_s);
}

