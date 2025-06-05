#include "tds.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define CHANNEL_NUMBER 0
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
#define TEMPERATURE 25.0

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

uint32_t count = 0;
uint16_t buf;
struct adc_sequence sequence;

bool init_tds(void) {
    int err;
	sequence = (struct adc_sequence) {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	/* Configure channels individually prior to sampling. */
    if (!adc_is_ready_dt(&adc_channels[CHANNEL_NUMBER])) {
        printk("ADC controller device %s not ready\n", adc_channels[CHANNEL_NUMBER].dev->name);
        return false;
    }

    err = adc_channel_setup_dt(&adc_channels[CHANNEL_NUMBER]);
    if (err < 0) {
        printk("Could not setup channel #%d (%d)\n", CHANNEL_NUMBER, err);
        return false;
    }

    return true;
}

// If this is inaccurate, could change this function to sample the TDS sensor multiple times
// returns TDS in ppm
int read_tds(void) {
    int32_t val_mv;
    int err;
    // printk("- %s, channel %d: ",
    //         adc_channels[CHANNEL_NUMBER].dev->name,
    //         adc_channels[CHANNEL_NUMBER].channel_id);

    (void)adc_sequence_init_dt(&adc_channels[CHANNEL_NUMBER], &sequence);

    err = adc_read_dt(&adc_channels[CHANNEL_NUMBER], &sequence);
    if (err < 0) {
        printk("Could not read adc (%d)\n", err);
        return -1;
    }

    /*
    * If using differential mode, the 16 bit value
    * in the ADC sample buffer should be a signed 2's
    * complement value.
    */
    if (adc_channels[CHANNEL_NUMBER].channel_cfg.differential) {
        val_mv = (int32_t)((int16_t)buf);
    } else {
        val_mv = (int32_t)buf;
    }

    // printk("%"PRId32, val_mv);
    err = adc_raw_to_millivolts_dt(&adc_channels[CHANNEL_NUMBER], &val_mv);
    /* conversion to mV may not be supported, skip if not */
    if (err < 0) {
        printk(" (value in mV not available)\n");
    } else {
        // printk(" = %"PRId32" mV\n", val_mv);
    }

    // Convert to PPM
    // https://wiki.dfrobot.com/Gravity__Analog_TDS_Sensor___Meter_For_Arduino_SKU__SEN0244

    float val_v = (float)val_mv / 1000.0F; // convert to voltage
    // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationCoefficient = 1.0+0.02*(TEMPERATURE-25.0);
    float compensationVoltage = val_v / compensationCoefficient; // temperature compensation
    // convert voltage value to tds value
    float tdsValue = (133.42F*compensationVoltage*compensationVoltage*compensationVoltage - 255.86F*compensationVoltage*compensationVoltage + 857.39F *compensationVoltage)*0.5F;

    return (int) tdsValue;
}