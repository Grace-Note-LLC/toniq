#include "imu.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(logging_imu);

// #define MY_TWIM DT_NODELABEL(i2c1)
// const struct device *nrfx_twis_dev1;

#define I2C0_NODE DT_NODELABEL(imu)
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NODE);

#define NUM_REG 6
#define GYRO_XOUT_H 0x43

#define GYRO_XOUT_H_IDX 0
#define GYRO_XOUT_L_IDX 1
#define GYRO_YOUT_H_IDX 2
#define GYRO_YOUT_L_IDX 3
#define GYRO_ZOUT_H_IDX 4
#define GYRO_ZOUT_L_IDX 5

static void i2c_init(void) {
     if (!device_is_ready(dev_i2c.bus)) {
        printk("I2C bus %s is not ready!\n\r",dev_i2c.bus->name);
        return;
    }


  // int config_result = false;
  //
  // nrfx_twis_dev1 = DEVICE_DT_GET(MY_TWIM);
  //
  // if (nrfx_twis_dev1 == NULL) {
  //   printk("\n\nI2C Slave: Device driver not found.\n");
  // } else {
  //   printk("\nI2C device 1: %s\n", DT_PROP(DT_NODELABEL(twis_device1), label));
  //
  //   config_result = i2c_configure(nrfx_twis_dev1, I2C_SPEED_SET(I2C_SPEED_FAST));
  //
  //   if (!config_result) {
  //     printk("I2C Master: Slave ADDR: 0x%x CLK(Hz): %u\n\n",
  //         DT_REG_ADDR(DT_NODELABEL(twis_device1)), 
  //         // DT_PROP(MY_TWIM, scl_pin),
  //         // DT_PROP(MY_TWIM, sda_pin),
  //         DT_PROP(MY_TWIM, clock_frequency));
  //   } else
  //     printk("\n\nI2C: Configuration error code: %d\n", config_result);
  // }
}


int init_imu(void) {
    // Set up IMU and register interrupt when bottle flipped over
    i2c_init();

    uint8_t pwr_mg[] = { 0x6B, 0 };

      int err;
      err = i2c_write_dt(&dev_i2c, pwr_mg, sizeof(pwr_mg));
      if (err != 0) {
          LOG_INF("dev_i2c %d\n", err);
          return -1;
      }

    return 0;
}

int16_t two_reg_to_deg_s(uint8_t high, uint8_t low) {
  int16_t deg_s = (high << 8) | low;
  // see page 31 for 250 deg/s range 
  // https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
  deg_s /= 131.0;
  return deg_s;
}

gyro_data_t read_imu(void) {
  int err;
  uint8_t rx_buf[NUM_REG];

  err = i2c_burst_read_dt(&dev_i2c, GYRO_XOUT_H, rx_buf, NUM_REG);
  if (err != 0) {
      gyro_data_t invalid;
      invalid.valid = false;
      return invalid;
  }

  int16_t x_deg_s = two_reg_to_deg_s(
          rx_buf[GYRO_XOUT_H_IDX], 
          rx_buf[GYRO_XOUT_L_IDX]);
  int16_t y_deg_s = two_reg_to_deg_s(
          rx_buf[GYRO_YOUT_H_IDX], 
          rx_buf[GYRO_YOUT_L_IDX]);
  int16_t z_deg_s = two_reg_to_deg_s(
          rx_buf[GYRO_ZOUT_H_IDX], 
          rx_buf[GYRO_ZOUT_L_IDX]);
  
  gyro_data_t raw_gyro = {
      .valid = true,
      .x_deg_s = x_deg_s,
      .y_deg_s = y_deg_s,
      .z_deg_s = z_deg_s,
  };

  return raw_gyro;
}
