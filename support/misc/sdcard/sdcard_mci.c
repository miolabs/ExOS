#include "sdcard_mci.h"
#include <support/mci_hal.h>

unsigned short _rca;

void sd_hw_initialize()
{
	hal_mci_initialize();
	_rca = 0;
}

void sd_hw_card_reset()
{
	// card initialization
	hal_mci_send_cmd(0, 0, 0);	// CMD0 (GO IDLE STATE)
	
	for(volatile int i = 0; i < 10000; i++);
}

int sd_hw_card_identification(void *cid, unsigned short *prca)
{
	SD_ERROR status = sd_send_cmd_resp(2, 0, cid, 16);	// CMD2: ALL_SEND_CID
	if (status == SD_OK)
	{	
		// state is CARD_IDENTIFICATION
		unsigned long r6;
		status = sd_send_cmd_resp(3, 0, &r6, 4);	// CMD3: SEND_RELATIVE_ADDR
		if (status == SD_OK)
		{
			_rca = r6 >> 16;	// high short in R6 
			*prca = _rca;
			return 1;
		}
	}
	return 0;
}

int sd_hw_select_card()
{
	unsigned long card_status;
	SD_ERROR status = sd_send_cmd_resp(7, _rca << 16, &card_status, 4); // CMD7: SELECT_CARD
	if (status >= 0)
	{
		// enable 4bit SD bus
		status = sd_send_acmd_resp(6, 2, &card_status, 4);	// 2 = 4bit bus
		if (status >= 0)
		{
         	hal_mci_set_speed(1);	// high speed mode (25MHz)
         	return 1;
		}
	}
	return 0;
}

SD_ERROR sd_send_cmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length)
{
	SD_ERROR error;
	switch(resp_length)
	{
		case 4:
			error = hal_mci_send_cmd(cmd, arg, MCI_WAIT_RESPONSE);
			*(unsigned long *)resp = hal_mci_read_short_resp();
			break;
		case 16:
			error = hal_mci_send_cmd(cmd, arg, MCI_WAIT_LONGRESP | MCI_WAIT_CRC);
			hal_mci_read_resp(resp, resp_length);
			break;
		default: return SD_ERROR_UNSUPPORTED;				
	}
	return error;
}

SD_ERROR sd_send_acmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length)
{
	SD_ERROR error = hal_mci_send_cmd(55, _rca << 16, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD);	// CMD55: APP_CMD
	if (error == SD_OK)
	{
		unsigned long card_status = hal_mci_read_short_resp();
		if (card_status & SD_CARD_STATUS_APP_CMD)
		{
			return sd_send_cmd_resp(cmd, arg, resp, resp_length);
		}
	}
	return SD_ERROR_NOT_IDLE;
}

SD_ERROR sd_send_op_cond(unsigned long arg, unsigned long *ocr_ptr)
{
	SD_ERROR status = sd_send_acmd_resp(41, arg | 0x00FF8000, ocr_ptr, 4); // ACMD41: SD_SEND_OP_COND
	if (status == SD_OK)
	{
		int ocr = *ocr_ptr;
		status = (ocr & SD_OCR_NOT_BUSY) ? SD_OK : SD_OK_IDLE;
	}
	return status;
}

SD_ERROR sd_hw_read_single_block(unsigned long addr, unsigned char *buf)
{
	SD_ERROR error = hal_mci_send_cmd(17, addr, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD); // CMD17: READ_SINGLE_BLOCK
	if (error == SD_OK)
	{
		error = hal_mci_read_data_blocks(buf, 1, MCI_BLOCK_512);
	}
    return error;
}

SD_ERROR sd_hw_read_blocks(unsigned long addr, unsigned long count, unsigned char *buf)
{
	int done = 0;
	SD_ERROR error1, error2;
	error1 = hal_mci_send_cmd(18, addr, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD);	// CMD18: READ_MULTIPLE_BLOCK
	if (error1 == SD_OK)
	{
		error1 = hal_mci_read_data_blocks(buf, count, MCI_BLOCK_512);
		error2 = hal_mci_send_cmd(12, 0, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD);
	}
	return error1 != SD_OK ? error1 : error2;
}

SD_ERROR sd_hw_write_single_block(unsigned long addr, unsigned char *buf)
{
	SD_ERROR error = hal_mci_send_cmd(24, addr, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD); // CMD24: WRITE_BLOCK
	if (error == SD_OK)
	{
		error = hal_mci_write_data_blocks(buf, 1, MCI_BLOCK_512);
	}
    return error;
}

SD_ERROR sd_hw_write_blocks(unsigned long addr, unsigned long count, unsigned char *buf)
{
	int done = 0;
	SD_ERROR error1, error2;
	error1 = hal_mci_send_cmd(25, addr, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD);	// CMD25: READ_MULTIPLE_BLOCK
	if (error1 == SD_OK)
	{
		error1 = hal_mci_write_data_blocks(buf, count, MCI_BLOCK_512);
		error2 = hal_mci_send_cmd(12, 0, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD);	// CMD12
	}
	return error1 != SD_OK ? error1 : error2;
}

SD_ERROR sd_hw_check_status(SD_CARD_STATE *pstate)
{
	SD_ERROR error = hal_mci_send_cmd(13, _rca << 16, MCI_WAIT_RESPONSE | MCI_WAIT_CRC | MCI_WAIT_CMD); // CMD13:SEND_STATUS
	if (error == SD_OK)
	{
		unsigned long card_status = hal_mci_read_short_resp();
		SD_CARD_STATE state = (card_status & SD_CARD_STATUS_STATE_MASK) >> SD_CARD_STATUS_STATE_BIT;
		*pstate = state;
	}
	return error;
}
