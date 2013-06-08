#include "apple_cp20.h"
#include <support/i2c_hal.h>
#include <kernel/thread.h>

#ifndef APPLE_CP20_I2C_MODULE
#define APPLE_CP20_I2C_MODULE 0
#endif

#ifndef APPLE_CP20_I2C_ADDRESS
#define APPLE_CP20_I2C_ADDRESS 0x10
#endif

static int _read(CP20_REG reg, unsigned char *buffer, int length);

int apple_cp20_initialize()
{
	int done;
	hal_i2c_initialize(APPLE_CP20_I2C_MODULE, 400000);

	unsigned char buffer[16];
	buffer[0] = CP20_REG_DEVICE_VERSION;

	exos_thread_sleep(10);
	done = _read(CP20_REG_DEVICE_VERSION, buffer, 4);
	done = _read(CP20_REG_DEVICE_ID, buffer, 4);
	done = _read(CP20_REG_ERROR_CODE, buffer, 1);

	return 0;
}

static int _read(CP20_REG reg, unsigned char *buffer, int length)
{
	int err;
	for (int retry = 0; retry < 3; retry++)
	{
		err = hal_i2c_master_frame(APPLE_CP20_I2C_MODULE, APPLE_CP20_I2C_ADDRESS, 
			(unsigned char *)&reg, 1, 0);
		if (err == 0) break;
		exos_thread_sleep(1);
	}
	if (err == 0)
	{
		for (int retry = 0; retry < 3; retry++)
		{
			err = hal_i2c_master_frame(APPLE_CP20_I2C_MODULE, APPLE_CP20_I2C_ADDRESS, 
				buffer, 0, length);
			if (err == 0) return 1;
			exos_thread_sleep(1);
		}
	}
	return 0;
}


