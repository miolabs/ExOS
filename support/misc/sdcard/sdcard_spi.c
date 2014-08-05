#include "sdcard_spi.h"
#include <support/ssp_hal.h>
#include <support/misc/crc16.h>
#include <kernel/thread.h>

static unsigned short _crc16_table[256];

void sd_hw_initialize()
{
	crc16_initialize(_crc16_table, 0x1021);
	
	hal_ssp_initialize(SDCARD_SPI_MODULE, 400000, HAL_SSP_MODE_SPI, HAL_SSP_CLK_IDLE_HIGH | HAL_SSP_IDLE_HIGH);

	sd_spi_power_control(1);
}

static inline unsigned char _transmit(unsigned char out)
{
	hal_ssp_transmit(SDCARD_SPI_MODULE, &out, &out, 1); 
	return out;
}

static void _flush()
{
	int i = 0;
	int j = 0;
	do
	{
		unsigned char inb = _transmit(0xFF);
		j = (inb == 0xFF) ? j + 1 : 0;
		if (++i >= 1000)
		{
			// FIXME: report this error
			break;
		}
	} while (j < 16);
}

static unsigned char _wait_token(unsigned char mask, unsigned char value)
{
	unsigned char resp;

	int timeout = 0;
	do  
	{
		resp = _transmit(0xFF);
		if (timeout++ >= SD_SPI_TOKEN_TIMEOUT) return SD_ERROR_TIMEOUT;
	} while ((resp & mask) != value);

	return resp;
}

static SD_ERROR _read_data(unsigned char *result, unsigned long length)
{
	if (length > 0)
	{
		unsigned char token = _wait_token(0x01, 0);
		if (token != SD_SPI_START_TOKEN) return SD_ERROR_BAD_TOKEN;
	
		hal_ssp_transmit(SDCARD_SPI_MODULE, 0, result, length);	// NOTE: unidirectional, this should transmit 0xFF
		unsigned short crc_rcv = (_transmit(0xFF) << 8) | _transmit(0xFF);
		unsigned short crc = 0;
		for(int i = 0; i < length; i++) CRC16(_crc16_table, crc, result[i]);
		if (crc != crc_rcv) return SD_ERROR_BAD_CRC;
	}
	return SD_OK;
}

static SD_ERROR _send_seq_r1(unsigned char *data, unsigned long length)
{
	unsigned char stuff1 = _transmit(0xFF);
	hal_ssp_transmit(SDCARD_SPI_MODULE, data, data, length);
	unsigned char stuff2 = _transmit(0xFF);
	unsigned char r1 = _wait_token(0x80, 0x00);
	return r1;
}

static SD_ERROR _send_cmd_r1(unsigned char cmd, unsigned long arg)
{
	unsigned char data6[] = {0x40 | (cmd & 0x3F), arg >> 24, arg >> 16, arg >> 8, arg, 0};
	return _send_seq_r1(data6, 6);
}

static SD_ERROR _send_cmd_r3(unsigned char cmd, unsigned long arg, unsigned long *result)
{
	unsigned char data6[] = {0x40 | (cmd & 0x3F), arg >> 24, arg >> 16, arg >> 8, arg, 0};
	if (cmd == 8) data6[5] = crc7_do(data6, 5);

	unsigned short r1 = _send_seq_r1(data6, 6);

	unsigned long r3 = (_transmit(0xFF) << 24) |
		(_transmit(0xFF) << 16) | 
		(_transmit(0xFF) << 8) | 
		_transmit(0xFF);
	*result = r3;

	int error;
	switch(r1)
	{
		case SD_SPI_R1_IDLE:	error = SD_OK_IDLE;	break;
		case SD_SPI_R1_BUSY:	error = SD_OK;	 break;
		default:	error = SD_ERROR_BAD_TOKEN;	break;
	}
	unsigned char idle = _transmit(0xFF);
	return (idle == 0xFF) ? error : SD_ERROR_NOT_IDLE;
}

int sd_hw_card_reset()
{
	_flush();
	
	// card initialization
	sd_spi_cs_assert();
	unsigned char cmd0[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };	// CMD0: GO_IDLE_STATE
	SD_ERROR state = _send_seq_r1(cmd0, 6);	// CMD0 + CS enables SPI mode
	sd_spi_cs_release();

	_transmit(0xFF);

	return (state == SD_OK_IDLE);
}

int sd_hw_card_identification(void *cid, unsigned short *prca)
{
	SD_ERROR status = sd_send_cmd_resp(10, 0, cid, 16);	// CMD10: SEND_CID
	return (status == SD_OK); 
}

int sd_hw_select_card()
{
	// change bit rate for high speed
	hal_ssp_initialize(SDCARD_SPI_MODULE, 25000000, HAL_SSP_MODE_SPI, HAL_SSP_CLK_IDLE_HIGH);
	return 1;
}

SD_ERROR sd_send_cmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length)
{
	// in SPI mode, responses are transfered as short resp (r1) followed by 4 or 16 bytes of data
	// but some commands are not supported in SPI mode or produce different response data

	sd_spi_cs_assert();
	SD_ERROR status;
	switch(resp_length)
	{
		case 4:
			status = _send_cmd_r3(cmd, arg, (unsigned long *)resp);
			break;
		case 0:
		case 16:
			status = _send_cmd_r1(cmd, arg);
			if (status == SD_OK)
			{
				status = _read_data(resp, resp_length);
			}
			break;
	}
    sd_spi_cs_release();
	return status;
}

SD_ERROR sd_send_acmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length)
{
	sd_spi_cs_assert();
	SD_ERROR status = _send_cmd_r1(55, 0);	// CMD55: APP_CMD
	if (status == SD_OK_IDLE)
	{
		status = sd_send_cmd_resp(cmd, arg, resp, resp_length);
	}
	else status = SD_ERROR_NOT_IDLE;
    sd_spi_cs_release();
	return status;
}

SD_ERROR sd_send_op_cond(unsigned long arg, unsigned long *ocr_ptr)
{
	SD_ERROR status = sd_send_acmd_resp(41, arg | 0x00FF8000, SD_NULL, 0);	// ACMD41: SD_SEND_OP_COND
	if (status == SD_OK)
	{
		status = sd_send_cmd_resp(58, 0, ocr_ptr, 4);	// CMD58: GET_CCS
	}
	return status;
}

SD_ERROR sd_hw_read_single_block(unsigned long addr, unsigned char *buf)
{
	sd_spi_cs_assert();
	SD_ERROR status = _send_cmd_r1(17, addr);	// CMD17: READ_SINGLE_BLOCK
	if (status == SD_OK)
	{
		status = _read_data(buf, 512);
	}
    sd_spi_cs_release();
	return status;
}

SD_ERROR sd_hw_read_blocks(unsigned long addr, unsigned long count, unsigned char *buf)
{
	sd_spi_cs_assert();
	SD_ERROR status = _send_cmd_r1(18, addr); // CMD18: READ_MULTIPLE_BLOCKS
	if (status == SD_OK)
	{
		for(int i = 0; i < count; i++)
		{
			status = _read_data(buf, 512);
			if (status != SD_OK) break;
			buf += 512;
		}
		status = _send_cmd_r1(12, 0);	// CMD12: STOP_TRANSMISSION
	}
    sd_spi_cs_release();
	return status;
}

static SD_ERROR _send_data(unsigned char start_token, unsigned char *data, unsigned long length)
{
	_transmit(0xFF);
	_transmit(start_token);
	// send data
	hal_ssp_transmit(SDCARD_SPI_MODULE, data, 0, length);
	unsigned short crc = 0;
	for(int i = 0; i < length; i++) CRC16(_crc16_table, crc, data[i]);
	// send crc16
	_transmit(crc >> 8);
	_transmit(crc);

	SD_ERROR status;
	int resp = _wait_token(0x11, 0x01);
	switch((resp & 0xe) >> 1)
	{
		case 0b010:	status = SD_OK;	break;
		case 0b101:	status = SD_ERROR_BAD_CRC; break;
		case 0b110:	status = SD_ERROR_WRITE_ERROR; break;
		default: status = SD_ERROR_BAD_TOKEN; break;
	}
	return status;
}

SD_ERROR sd_hw_write_single_block(unsigned long addr, unsigned char *buf)
{
    sd_spi_cs_assert();
	SD_ERROR status = _send_cmd_r1(24, addr);	// CMD24: WRITE_SINGLE_BLOCK
	if (status == SD_OK)
	{
		status = _send_data(SD_SPI_START_TOKEN, buf, 512);
	}
	sd_spi_cs_release();
	return status;
}

SD_ERROR sd_hw_write_blocks(unsigned long addr, unsigned long count, unsigned char *buf)
{
    sd_spi_cs_assert();
	SD_ERROR status = _send_cmd_r1(25, addr); // CMD25: WRITE_MULTIPLE_BLOCKS
	if (status == SD_OK)
	{
		while(count--)
		{
			status = _send_data(SD_SPI_START_WRITE_MULTI_TOKEN, buf, 512);
			if (status != SD_OK) break;
			buf += 512;
			while(0xFF != _transmit(0xFF));	// wait while busy
		}
		if (status == SD_OK) 
			_transmit(SD_SPI_STOP_TRAN_TOKEN);	// STOP_TRAN_TOKEN
	}
	sd_spi_cs_release();
	return status;
}

SD_ERROR sd_hw_check_status(SD_CARD_STATE *pstate)
{
	if (*pstate == SD_CARD_PROGRAMMING)
	{
		sd_spi_cs_assert();
		unsigned char stuff = _transmit(0xFF);
		unsigned char idle = _transmit(0xFF);
		sd_spi_cs_release();
		if (idle == 0xFF)
			*pstate = SD_CARD_TRANSFER;
	}
	return SD_OK;
}

