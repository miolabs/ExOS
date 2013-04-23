#include "eeprom.h"
#include <support/i2c_hal.h>
#include <support/board_hal.h>

#ifndef EEPROM_I2C_ADDRESS
#define EEPROM_I2C_ADDRESS 0x50
#endif

#ifndef EEPROM_I2C_CLOCK
#define EEPROM_I2C_CLOCK 400000
#endif

#ifndef EEPROM_PAGE_SIZE // must be a power of 2
#define EEPROM_PAGE_SIZE 64
#endif

int eeprom_initialize()
{
	hal_i2c_initialize(EEPROM_I2C_MODULE, EEPROM_I2C_CLOCK);
	return 1;	// FIXME: check i2c port availability
}

int eeprom_read_geometry(int *psize, int *ppagesize)
{
	*psize = EEPROM_SIZE;
	*ppagesize = EEPROM_PAGE_SIZE;
	return 1;
}

EEPROM_RESULT eeprom_read(unsigned char *buf, int offset, int length)
{
	EEPROM_RESULT error = 0;
	unsigned char frame[2 + EEPROM_PAGE_SIZE];
	while (length > 0)
	{
		frame[0] = (unsigned char)(offset >> 8);
		frame[1] = (unsigned char)offset;
		int max_length = EEPROM_PAGE_SIZE - (offset & (EEPROM_PAGE_SIZE - 1));
		int frame_length = length > max_length ? max_length : length;
		error = hal_i2c_master_frame(EEPROM_I2C_MODULE, EEPROM_I2C_ADDRESS, frame, 2, frame_length);
		if (error == EEPROM_RES_OK)
		{
			for (int i = 0; i < frame_length; i++) *buf++ = frame[i + 2];
			offset += frame_length;
			length -= frame_length;
		}
		else break;
	}
	return error;
}

EEPROM_RESULT eeprom_write(unsigned char *buf, int offset, int length)
{
	EEPROM_RESULT error = 0;
	unsigned char frame[2 + EEPROM_PAGE_SIZE];
	while(length > 0)
	{
		frame[0] = (unsigned char)(offset >> 8);
		frame[1] = (unsigned char)offset;
		int max_length = EEPROM_PAGE_SIZE - (offset & (EEPROM_PAGE_SIZE - 1));
		int frame_length = length > max_length ? max_length : length;
		for (int i = 0; i < frame_length; i++) frame[i + 2] = *buf++;
		error = hal_i2c_master_frame(EEPROM_I2C_MODULE, EEPROM_I2C_ADDRESS, frame, 2 + frame_length, 0);
		if (error == 3)
		{
			for(int retry = 0; retry < 200; retry++)
			{
				error = hal_i2c_master_frame(EEPROM_I2C_MODULE, EEPROM_I2C_ADDRESS, frame, 2 + frame_length, 0);
				if (error != 3) 
					break;
			}
		}
		if (error != 0) break;
		offset += frame_length;
		length -= frame_length;
	}
	return error;
}

__attribute__((__weak__))
void eeprom_lock_i2c()
{
}

__attribute__((__weak__))
void eeprom_unlock_i2c()
{
}


