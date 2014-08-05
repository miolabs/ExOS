// SD/SDHD Memory Card Support
// by Miguel Fides

#include "sdcard.h"

static SD_INFO _info;
static unsigned short _rca; // relative card address (not used in SPI mode)
static unsigned short _card_version;
static unsigned short _card_hc;
static SD_CARD_STATE _state = SD_CARD_DISCONNECT;

static void _copy_flipped(unsigned char *src, unsigned char *dst, int size);

int sd_initialize()
{
	SD_ERROR status;

	_info.Blocks = 0;
	_info.BlockSize = 0;
	_state = SD_CARD_DISCONNECT;

	sd_hw_initialize();

	if (!sd_hw_card_reset())
		return 0;
	
	_state = SD_CARD_IDLE;
	_card_version = 100;
	_card_hc = 0;

	// send CMD8 to detect V2.0 cards
	#define VHS_2_7_TO_3_6 1
	unsigned char r7[4];
	unsigned long cmd8_arg = (VHS_2_7_TO_3_6 << 8) | 0xAA;
	status = sd_send_cmd_resp(8, cmd8_arg, r7, 4);	// CMD8: SEND_IF_COND
	if (status == SD_OK || status == SD_OK_IDLE)
	{
		_card_version = 200;
	}

	do
	{
		// go into IDENT_MODE
		// send ACMD41 (OP_COND) until it is ready
		unsigned long arg = (_card_version >= 200) ? (1<<30) : 0;
		status = sd_send_op_cond(arg, &(_info.OCR));
	} while (status == SD_OK_IDLE);
	if (status == SD_OK)
	{
		if ((_card_version >= 200) && (_info.OCR & SD_OCR_HCS)) _card_hc = 1;
		_state = SD_CARD_READY;
	}

	if (_state == SD_CARD_READY) // we got OP_COND!
	{
		if (sd_hw_card_identification(_info.CID, &_rca))
			_state = SD_CARD_STAND_BY;
	}

	if (_state == SD_CARD_STAND_BY)
	{
		// read card geometry
		status = sd_send_cmd_resp(9, _rca << 16, _info.CSD, 16);	// CMD9: SEND_CSD
		if (status == SD_OK) 
		{
			SD_CSD_FLIPPED csd;
			_copy_flipped(_info.CSD, (unsigned char *)&csd, 16);
		
			if (_card_hc)
			{
				_info.Blocks = ((csd.C_SIZEF >> 1) + 1) * 1024;
				_info.BlockSize = 512;
			}
			else
			{
				_info.Blocks = ((csd.C_SIZEF >> 15) + 1) * (1 << ((csd.C_SIZEF & 7) + 2));
				_info.BlockSize = 1 << csd.READ_BL_LEN;
			}
		}

		if (_info.Blocks == 0 || _info.BlockSize == 0)
		{
			// geometry unrecognized
			_state = SD_CARD_DISCONNECT;
		}
	}

	if (_state == SD_CARD_STAND_BY)
	{
		_state = sd_hw_select_card(_rca) ?
			SD_CARD_TRANSFER :
			SD_CARD_DISCONNECT;
	}

	if (_state == SD_CARD_TRANSFER)
	{
		// TODO: add device
		return 1;
	}
	return 0;
}

// copies an array of bytes backwards to allow processing of Big Endian Bit Fields (OMG)
static void _copy_flipped(unsigned char *src, unsigned char *dst, int size)
{
	for (int i = 0; i < size; i++) dst[i] = src[size - i - 1];
}

void sd_get_info(SD_INFO *info)
{
	// copy structure
	*info = _info;
}

SD_ERROR sd_read_blocks(unsigned long block, unsigned long count, unsigned char *buf)
{
	if (_state == SD_CARD_PROGRAMMING)
	{
		SD_ERROR status = sd_hw_check_status(&_state);
		if (status != SD_OK) return status;
	}
	if (_state != SD_CARD_TRANSFER) return SD_ERROR_BAD_STATE;
	unsigned long addr = _card_hc ? block : (block << 9);
	return (count == 1) ? 
		sd_hw_read_single_block(addr, buf) :
		sd_hw_read_blocks(addr, count, buf);
}

SD_ERROR sd_write_blocks(unsigned long block, unsigned long count, unsigned char *buf)
{
	while (_state == SD_CARD_PROGRAMMING || _state == SD_CARD_RECEIVE_DATA)
	{
		SD_ERROR status = sd_hw_check_status(&_state);
		if (status != SD_OK) return status;
	}
	if (_state != SD_CARD_TRANSFER) 
		return SD_ERROR_BAD_STATE;
	
	unsigned long addr = _card_hc ? block : (block << 9);
	SD_ERROR status = (count == 1) ? 
		sd_hw_write_single_block(addr, buf) :
		sd_hw_write_blocks(addr, count, buf);
	if (status == SD_OK) 
	{
		_state = SD_CARD_PROGRAMMING;
	}
	return status;
}

SD_ERROR sd_get_card_state(SD_CARD_STATE *pstate)
{
	SD_ERROR status = sd_hw_check_status(&_state);
	*pstate = _state;
	return status;
}





