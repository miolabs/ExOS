#include "pca9635.h"
#include <support/i2c_hal.h>

#ifndef PCA9635_I2C_PORT
#define PCA9635_I2C_PORT 0
#endif

#ifndef PCA9635_ADDRESS
#define PCA9635_ADDRESS 0x1F
#endif

static unsigned char _output[4];

static int _reg_write(PCA9635_REGISTER reg, unsigned char value)
{
	unsigned char temp[] = { reg, value };
	return hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 2, 0);
}

static int _reg_read(PCA9635_REGISTER reg, unsigned char *pvalue)
{
	unsigned char temp[] = { reg, 0 };
	int error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 1, 1);
	if (error == 0) *pvalue = temp[1];
	return error;
}

int pca9635_initialize(PCA9635_MODE mode)
{
	unsigned char temp[6];
	int error;
#if 0
	temp[0] = 0xa5;	 // software reset
	temp[1] = 0x5a;
	error = hal_i2c_master_frame(PCA9635_I2C_PORT, 3, temp, 2, 0);
	if (error != 0) return 0;
#endif
	error = _reg_write(PCA9635_MODE2, mode);
	if (error != 0) return 0;
	
	temp[0] = 0x80 | PCA9635_LEDOUT0;
	error = hal_i2c_master_frame(PCA9635_I2C_PORT, PCA9635_ADDRESS, temp, 1, 4);
	if (error != 0) return 0;

	_output[0] = temp[1];
	_output[1] = temp[2];
	_output[2] = temp[3];
	_output[3] = temp[4];
	return 1;
}

int pca9635_set_output(int index, PC9635_OUTPUT_STATE state)
{
	int shift = (index & 0x3) << 1;
	int offset = index >> 2;
	_output[offset] = (_output[offset] & ~(0x03 << shift)) | (state << shift);
	int error = _reg_write(PCA9635_LEDOUT0 + offset, _output[offset]);
	return error == 0;
}

int pca9635_sel_output_pwm(int index, unsigned char duty)
{
	int error = _reg_write(PCA9635_PWM0 + index, duty);
	return error == 0;
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




