#include "pca9674.h"
#include <support/i2c_hal.h>

#define PCA_ADDRESS 0x20 // for AD0=AD1=AD2=0
#define PCA_DEVICE_ID_ADDRESS 0x7c // reserved device id address

int pca9674_initialize(int port)
{
	hal_i2c_initialize(port, 400000);

	unsigned char temp[6];
	
	temp[0] = 6;	 // software reset
	int error = hal_i2c_master_frame(port, HAL_I2C_GENERAL_CALL_ADDR, temp, 1, 0);

	temp[0] = PCA_ADDRESS << 1;
	error = hal_i2c_master_frame(port, PCA_DEVICE_ID_ADDRESS, temp, 1, 3);
	if (error == 0)
	{
		PCA9674_DEVICE_ID device = (PCA9674_DEVICE_ID) { .Bytes = { temp[3], temp[2], temp[1] } };
		if (device.Manufacturer == 0
			&& device.Category == 1
			&& device.Feature == 11)
		{
			return 1;
		}
	}
	return 0;
}

int pca9674_write(int port, unsigned char value)
{
	unsigned char temp[] = { value };
	int error = hal_i2c_master_frame(port, PCA_ADDRESS, temp, 1, 0);
	return error == 0;
}

int pca9674_read(int port, unsigned char *pvalue)
{
	int error = hal_i2c_master_frame(port, PCA_ADDRESS, pvalue, 0, 1);
	return error == 0;
}




