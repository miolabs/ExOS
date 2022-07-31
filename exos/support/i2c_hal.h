#ifndef HAL_I2C_HAL_H
#define HAL_I2C_HAL_H

#define HAL_I2C_GENERAL_CALL_ADDR 0

// prototypes
void hal_i2c_initialize(int module, int bitrate);
int hal_i2c_master_frame(int module, unsigned char slave, 
	unsigned char *buffer, int write_len, int read_len);

// hooks
void hal_i2c_mutex_enter(int module) __attribute__((__weak__));
void hal_i2c_mutex_exit(int module) __attribute__((__weak__));

#endif // HAL_I2C_HAL_H

