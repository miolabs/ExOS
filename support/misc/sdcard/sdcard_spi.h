#ifndef SDCARD_SPI_H
#define SDCARD_SPI_H

#include "sdcard.h"

typedef enum
{
	SD_SPI_R1_BUSY = 0,
	SD_SPI_R1_IDLE = (1<<0),
	SD_SPI_R1_ERASE_RESET = (1<<1),
	SD_SPI_R1_ILLEGAL = (1<<2),
	SD_SPI_R1_ERASE_SEQ_ERROR = (1<<4),
	SD_SPI_R1_ADDRESS_ERROR = (1<<5),
	SD_SPI_R1_PARAM_ERROR = (1<<6),
} SD_SPI_R1;

#define SD_SPI_START_TOKEN 0xFE
#define SD_SPI_START_WRITE_MULTI_TOKEN 0xFC
#define SD_SPI_STOP_TRAN_TOKEN 0xFD
#define SD_SPI_TOKEN_TIMEOUT 5000

// board specific
void sd_spi_power_control(int power) __attribute__((__weak__));
void sd_spi_cs_assert();
void sd_spi_cs_release();

#endif // SDCARD_SPI_H
