#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdlib.h>
// #include "bluetooth.h"
// #include "tds.h"
// #include "ultrasonic.h"

#define SPI1_NODE DT_NODELABEL(spi1)
static const struct device* spi1_dev = DEVICE_DT_GET(SPI1_NODE);

const struct device* gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#define GPIO_0_DC 21
#define GPIO_0_BUSY 11
#define GPIO_0_RST 12
#define GPIO_0_PWR 14

#define EPD_RST_PIN GPIO_0_RST
#define EPD_DC_PIN GPIO_0_DC
#define EPD_BUSY_PIN GPIO_0_BUSY
#define EPD_PWR_PIN GPIO_0_PWR

const struct device* gpio1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
#define GPIO_1_CS 7

#define EPD_CS_PIN GPIO_1_CS
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0

// Display dimensions (212x104 for D variant)
#define EPD_WIDTH 104 // Width is the smaller dimension for buffer calculation
#define EPD_HEIGHT 212 // Height is the larger dimension
#define EPD_2IN13D_WIDTH (EPD_WIDTH)
#define EPD_2IN13D_HEIGHT (EPD_HEIGHT)
#define EPD_BUFFER_SIZE ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8 * EPD_HEIGHT) : ((EPD_WIDTH / 8 + 1) * EPD_HEIGHT))

#define DEV_DIGITAL_WRITE(_pin, _value) gpio_pin_set(gpio1_dev, _pin, _value == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET)
#define DEV_Delay_ms(x) k_msleep(x)

#define UWORD uint16_t
#define UBYTE uint8_t

// static uint8_t epd_buffer[EPD_BUFFER_SIZE];

static struct spi_config spi_cfg = {
	.frequency = 125000U,
	.operation = SPI_WORD_SET(8),
	.slave = 0,
};

static int gpio_setup() {
	if (!device_is_ready(spi1_dev)) {
		printk("spi1_dev not ready\n");
		return -1;
	}

	if (!device_is_ready(gpio0_dev)) {
		printk("gpio0_dev not ready\n");
		return -1;
	}

	if (!device_is_ready(gpio1_dev)) {
		printk("gpio1_dev not ready\n");
		return -1;
	}

	int err;
	err = gpio_pin_configure(gpio1_dev, GPIO_1_CS, GPIO_OUTPUT);
	if (err < 0) {
		return -1;
	}
	err = gpio_pin_configure(gpio0_dev, GPIO_0_DC, GPIO_OUTPUT);
	if (err < 0) {
		return -1;
	}
	err = gpio_pin_configure(gpio0_dev, GPIO_0_BUSY, GPIO_INPUT);
	if (err < 0) {
		return -1;
	}
	err = gpio_pin_configure(gpio0_dev, GPIO_0_RST, GPIO_OUTPUT);
	if (err < 0) {
		return -1;
	}
	err = gpio_pin_configure(gpio0_dev, GPIO_0_PWR, GPIO_OUTPUT);
	if (err < 0) {
		return -1;
	}
	return 0;
}


static void EPD_2IN13D_SendCommand(uint8_t reg) {
	int err;

	uint8_t tx_values[] = {reg};
	struct spi_buf tx_spi_bufs[] = {
		{ .buf = tx_values, .len = sizeof(tx_values) }
	};
	struct spi_buf_set spi_tx_buffer_set = {
		.buffers = tx_spi_bufs,
		.count = 1
	};

	gpio_pin_set(gpio0_dev, GPIO_0_DC, 0);
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 0);
	err = spi_write(spi1_dev, &spi_cfg, &spi_tx_buffer_set); 
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

	if (err < 0) {
		printk("set_register failed: %d\n", err);
	}
}

static void EPD_2IN13D_SendData(uint8_t reg) {
	int err;

	uint8_t tx_values[] = {reg};
	struct spi_buf tx_spi_bufs[] = {
		{ .buf = tx_values, .len = sizeof(tx_values) }
	};
	struct spi_buf_set spi_tx_buffer_set = {
		.buffers = tx_spi_bufs,
		.count = 1
	};

	gpio_pin_set(gpio0_dev, GPIO_0_DC, 1);
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 0);
	err = spi_write(spi1_dev, &spi_cfg, &spi_tx_buffer_set); 
	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

	if (err < 0) {
		printk("set_register failed: %d\n", err);
	}
}

static void set_register(uint8_t reg, uint8_t value) {
	int err;

	uint8_t tx_values[] = {reg, value};

	struct spi_buf tx_spi_bufs[] = {
		{ .buf = tx_values, .len = sizeof(tx_values) }
	};

	struct spi_buf_set spi_tx_buffer_set = {
		.buffers = tx_spi_bufs,
		.count = 1
	};

	gpio_pin_set(gpio1_dev, GPIO_1_CS, 0);
	err = spi_write(spi1_dev, &spi_cfg, &spi_tx_buffer_set); 
	// HAL_SPI_Transmit(&hspi1, &value, 1, 1000);
	// hspi1= spi1_dev, value = spi_tx_buffer_set, 1 = size of data, 1000 = timeout
	// DEV_SPI_WriteByte(value) = spi_write(spi1_dev, &spi_cfg, &spi_tx_buffer_set);  where value is nested inside two structs
	
	// #define DEV_DIGITAL_WRITE(_pin, _value) HAL_GPIO_WritePin(_pin, _value == 0? GPIO_PIN_RESET:GPIO_PIN_SET)
	// gpio_pin_set(gpio1_dev, _pin, _value == 0 ? GPIO_PIN_RESET=0 : GPIO_PIN_SET=1);

	gpio_pin_set(gpio1_dev, GPIO_1_CS, 1);

	if (err < 0) {
		printk("set_register failed: %d\n", err);
	}
}

static void EPD_2IN13D_TurnOnDisplay(void)
{
    EPD_2IN13D_SendCommand(0x12);		 //DISPLAY REFRESH
    k_msleep(10);

    EPD_2IN13D_ReadBusy();
}

static void EPD_2IN13D_Reset(void)
{
    DEV_DIGITAL_WRITE(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_DIGITAL_WRITE(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_DIGITAL_WRITE(EPD_RST_PIN, 1);
    DEV_Delay_ms(200);
}

static void EPD_2IN13D_ReadBusy(void)
{
    printk("e-Paper busy\r\n");
	DEV_Delay_ms(100);
    while(gpio_pin_get(gpio0_dev, EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
        DEV_Delay_ms(100);
    }
    printk("e-Paper busy release\r\n");
}

int DEV_Module_Init(void)
{
	int err;
	err = DEV_Digital_Write(EPD_DC_PIN, 0);
	if (err < 0) {
		printk("Failed to write to EPD_DC_PIN\n");
		return err;
	}

	err = DEV_Digital_Write(EPD_CS_PIN, 0);
	if (err < 0) {
		printk("Failed to write to EPD_CS_PIN\n");
		return err;
	}

	err = DEV_Digital_Write(EPD_PWR_PIN, 1);
	if (err < 0) {
		printk("Failed to write to EPD_PWR_PIN\n");
		return err;
	}

	err = DEV_Digital_Write(EPD_RST_PIN, 1);
	if (err < 0) {
		printk("Failed to write to EPD_RST_PIN\n");
		return err;
	}

	return 0;
}

void EPD_2IN13D_Init() {


    EPD_2IN13D_Reset();

    EPD_2IN13D_SendCommand(0x01);	//POWER SETTING
    EPD_2IN13D_SendData(0x03);
    EPD_2IN13D_SendData(0x00);
    EPD_2IN13D_SendData(0x2b);
    EPD_2IN13D_SendData(0x2b);
    EPD_2IN13D_SendData(0x03);

    EPD_2IN13D_SendCommand(0x06);	//boost soft start
    EPD_2IN13D_SendData(0x17);     //A
    EPD_2IN13D_SendData(0x17);     //B
    EPD_2IN13D_SendData(0x17);     //C

    EPD_2IN13D_SendCommand(0x04);
    EPD_2IN13D_ReadBusy();

    EPD_2IN13D_SendCommand(0x00);	//panel setting
    EPD_2IN13D_SendData(0xbf);     //LUT from OTP，128x296
    EPD_2IN13D_SendData(0x0e);     //VCOM to 0V fast

    EPD_2IN13D_SendCommand(0x30);	//PLL setting
    EPD_2IN13D_SendData(0x3a);     // 3a 100HZ   29 150Hz 39 200HZ	31 171HZ

    EPD_2IN13D_SendCommand(0x61);	//resolution setting
    EPD_2IN13D_SendData(EPD_2IN13D_WIDTH);
    EPD_2IN13D_SendData((EPD_2IN13D_HEIGHT >> 8) & 0xff);
    EPD_2IN13D_SendData(EPD_2IN13D_HEIGHT& 0xff);

    EPD_2IN13D_SendCommand(0x82);	//vcom_DC setting
    EPD_2IN13D_SendData(0x28);
}

void EPD_2IN13D_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_2IN13D_WIDTH % 8 == 0)? (EPD_2IN13D_WIDTH / 8 ): (EPD_2IN13D_WIDTH / 8 + 1);
    Height = EPD_2IN13D_HEIGHT;

    EPD_2IN13D_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN13D_SendData(0x00);
        }
    }

    EPD_2IN13D_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN13D_SendData(0xFF);
        }
    }

    EPD_2IN13D_SetFullReg();
    EPD_2IN13D_TurnOnDisplay();
}

static const unsigned char EPD_2IN13D_lut_vcomDC[] = {
    0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x60, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
};
static const unsigned char EPD_2IN13D_lut_ww[] = {
    0x40, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x40, 0x14, 0x00, 0x00, 0x00, 0x01,
    0xA0, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_2IN13D_lut_bw[] = {
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x0F, 0x0F, 0x00, 0x00, 0x03,
    0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
    0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_2IN13D_lut_wb[] = {
    0x80, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x80, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x50, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned char EPD_2IN13D_lut_bb[] = {
    0x80, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x80, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x50, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void EPD_2IN13D_SetFullReg(void)
{
    EPD_2IN13D_SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
    EPD_2IN13D_SendData(0xb7);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

    unsigned int count;
    EPD_2IN13D_SendCommand(0x20);
    for(count=0; count<44; count++) {
        EPD_2IN13D_SendData(EPD_2IN13D_lut_vcomDC[count]);
    }

    EPD_2IN13D_SendCommand(0x21);
    for(count=0; count<42; count++) {
        EPD_2IN13D_SendData(EPD_2IN13D_lut_ww[count]);
    }

    EPD_2IN13D_SendCommand(0x22);
    for(count=0; count<42; count++) {
        EPD_2IN13D_SendData(EPD_2IN13D_lut_bw[count]);
    }

    EPD_2IN13D_SendCommand(0x23);
    for(count=0; count<42; count++) {
        EPD_2IN13D_SendData(EPD_2IN13D_lut_wb[count]);
    }

    EPD_2IN13D_SendCommand(0x24);
    for(count=0; count<42; count++) {
        EPD_2IN13D_SendData(EPD_2IN13D_lut_bb[count]);
    }
}

void EPD_2IN13D_Display(UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_2IN13D_WIDTH % 8 == 0)? (EPD_2IN13D_WIDTH / 8 ): (EPD_2IN13D_WIDTH / 8 + 1);
    Height = EPD_2IN13D_HEIGHT;

    EPD_2IN13D_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN13D_SendData(0x00);
        }
    }
    // Dev_Delay_ms(10);

    EPD_2IN13D_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN13D_SendData(Image[i + j * Width]);
        }
    }
    // Dev_Delay_ms(10);

	EPD_2IN13D_SetFullReg();
    EPD_2IN13D_TurnOnDisplay();
}

int main(void) {
	printk("starting up...\n");

	int err; 

	err = gpio_setup();
	if (err < 0) {
		printk("gpio_setup failed\n");
		return -1;
	}

	err = DEV_Module_Init();
	if (err != 0) {
		printk("DEV_Module_Init failed\n");
		return -1;
	}

	printk("gpio setup done\n");

	DEV_Delay_ms(1000);
	EPD_2IN13D_Init();
	EPD_2IN13D_Clear();
	DEV_Delay_ms(1000);

	// drawing images starts here
    UBYTE *BlackImage;
    /* you have to edit the prj.conf file and set a big enough heap size */
    UWORD Imagesize = ((EPD_2IN13D_WIDTH % 8 == 0)? (EPD_2IN13D_WIDTH / 8 ): (EPD_2IN13D_WIDTH / 8 + 1)) * EPD_2IN13D_HEIGHT;

	printfk("Allocating %d bytes of memory for BlackImage...\r\n", Imagesize);

    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
	// Initialize BlackImage with a checkerboard pattern
	for (UWORD j = 0; j < EPD_2IN13D_HEIGHT; j++) {
		for (UWORD i = 0; i < EPD_2IN13D_WIDTH / 8; i++) {
			if ((j / 8) % 2 == 0) {
				BlackImage[i + j * (EPD_2IN13D_WIDTH / 8)] = (i % 2 == 0) ? 0xAA : 0x55; // Alternating pattern
			} else {
				BlackImage[i + j * (EPD_2IN13D_WIDTH / 8)] = (i % 2 == 0) ? 0x55 : 0xAA; // Inverse pattern
			}
		}
	}

	// Display the checkerboard pattern
	EPD_2IN13D_Display(BlackImage);

	while (true) {
		// set_register(0x69, 0x69);
		printk("kms)\n");
		k_msleep(3000);
	}

	return 0;
}
