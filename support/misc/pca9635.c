#include "pca9635.h"
#include <support/i2c_hal.h>

#ifndef PCA9635_I2C_PORT
#define PCA9635_I2C_PORT 0
#endif

#ifndef PCA9635_ADDRESS
#define PCA9635_ADDRESS 0x1F
#endif

int pca9635_initialize(PCA9635_MODE mode)
{
	unsigned char temp[6];
	
	temp[0] = 0x85;	 // software reset
	temp[1] = 0x5a;
	int error = hal_i2c_master_frame(PCA9635_I2C_PORT, 3, temp, 1, 0);

	temp[0] = PCA9635_MODE2;
	error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 1, 1);
	temp[1] = mode;
	error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 2, 0);

//	temp[0] = 0x80 | PCA9635_LEDOUT0;
//	error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 1, 4);

	return 0;
}

//int pca9635_write(unsigned char value)
//{
//	unsigned char temp[] = { value };
//	int error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 1, 0);
//	return error == 0;
//}
//
//int pca9635_read(unsigned char *pvalue)
//{
//	int error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, pvalue, 0, 1);
//	return error == 0;
//}




