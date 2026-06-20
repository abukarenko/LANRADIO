#pragma once

#include "driver/gpio.h"

/* Board-specific wiring. Defaults are for a W5500 SPI Ethernet module. */
#define LANRADIO_W5500_HOST             SPI2_HOST
#define LANRADIO_W5500_MOSI_GPIO        GPIO_NUM_11
#define LANRADIO_W5500_MISO_GPIO        GPIO_NUM_13
#define LANRADIO_W5500_SCLK_GPIO        GPIO_NUM_12
#define LANRADIO_W5500_CS_GPIO          GPIO_NUM_10
#define LANRADIO_W5500_INT_GPIO         GPIO_NUM_9
#define LANRADIO_W5500_RST_GPIO         GPIO_NUM_8
#define LANRADIO_W5500_SPI_HZ           (20 * 1000 * 1000)

/* External PCM DAC, ESP32-S3 I2S TX. */
#define LANRADIO_I2S_PORT               0
#define LANRADIO_I2S_BCLK_GPIO          GPIO_NUM_14
#define LANRADIO_I2S_WS_GPIO            GPIO_NUM_15
#define LANRADIO_I2S_DOUT_GPIO          GPIO_NUM_16

#define LANRADIO_UART_NUM               0
#define LANRADIO_UART_BAUD              115200
#define LANRADIO_STREAM_BUFFER_BYTES    (32 * 1024)
