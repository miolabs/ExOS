#include "cp20.h"
#include <support/i2c_hal.h>
#include <kernel/thread.h>

#ifndef APPLE_CP20_I2C_MODULE
#define APPLE_CP20_I2C_MODULE 0
#endif

static int _addr;

int apple_cp20_initialize()
{
	unsigned long auth_id;
	int done;
	hal_i2c_initialize(APPLE_CP20_I2C_MODULE, 400000);
	
	_addr = 0x10;
	done = apple_cp2_read_device_id(&auth_id);
	if (done && auth_id == 0x200) return 1;
	_addr = 0x11;
	done = apple_cp2_read_device_id(&auth_id);
	if (done && auth_id == 0x200) return 1;
	return 0;
}

static int _read(CP20_REG reg, unsigned char *buffer, unsigned char length)
{
	int err;
	for (int retry = 0; retry < 3; retry++)
	{
		err = hal_i2c_master_frame(APPLE_CP20_I2C_MODULE, _addr, 
			(unsigned char *)&reg, 1, 0);
		if (err == 0) break;
		exos_thread_sleep(1);
	}
	if (err == 0)
	{
		for (int retry = 0; retry < 10; retry++)
		{
			err = hal_i2c_master_frame(APPLE_CP20_I2C_MODULE, _addr, 
				buffer, 0, length);
			if (err == 0) return 1;
			exos_thread_sleep(1);
		}
	}
	return 0;
}

static int _write(CP20_REG reg, unsigned char *buffer, unsigned char length)
{
	int err;
	unsigned char buf2[length + 1];
	buf2[0] = reg;
	for (int i = 0; i < length; i++) buf2[i + 1] = buffer[i];

	for (int retry = 0; retry < 10; retry++)
	{
		err = hal_i2c_master_frame(APPLE_CP20_I2C_MODULE, _addr, 
			buf2, length + 1, 0);
		if (err == 0) return 1;
		exos_thread_sleep(1);
	}
	return 0;
}

int apple_cp2_read_device_id(unsigned long *pdevice_id)
{
	int done;
    unsigned char buffer[4];
	done = _read(CP20_REG_DEVICE_ID, buffer, 4);
	if (done)
	{
		*pdevice_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
		return 1;
	}
	return 0;
}

int apple_cp2_read_acc_cert_length(unsigned short *plength)
{
	int done;
    unsigned char buffer[2];
	done = _read(CP20_REG_ACCESORY_CERTIFICATE_DATA_LENGTH, buffer, 2);
	if (done)
	{
		*plength = (buffer[0] << 8) | buffer[1];
		return 1;
	}
	return 0;
}

int apple_cp2_read_acc_cert_page(int page, unsigned char *buffer, int length)
{
	int done;
	done = _read(CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE1 + page, buffer, length);
	return done;
}

int apple_cp2_get_auth_signature(unsigned char *challenge, int ch_len, unsigned char *sig)
{
	int done;
	unsigned char buffer[2];

	done = _read(CP20_REG_ERROR_CODE, buffer, 1);
	buffer[0] = 0;
	done = _write(CP20_REG_AUTH_CONTROL_AND_STATUS, buffer, 1);
	done = _read(CP20_REG_AUTH_CONTROL_AND_STATUS, buffer, 1);
	done = _read(CP20_REG_ERROR_CODE, buffer, 1);

	done = _write(CP20_REG_CHALLENGE_DATA, challenge, ch_len);
	if (done)
	{
		buffer[0] = ch_len >> 8;
		buffer[1] = ch_len;
		done = _write(CP20_REG_CHALLENGE_DATA_LENGTH, buffer, 2);
		if (done)
		{
			done = _read(CP20_REG_AUTH_CONTROL_AND_STATUS, buffer, 1);

			buffer[0] = CP20_PROC_START_SIGNATURE_GENERATION;
			done = _write(CP20_REG_AUTH_CONTROL_AND_STATUS, buffer, 1);

			for(int i = 0; i < 15; i++)
			{
				done = _read(CP20_REG_AUTH_CONTROL_AND_STATUS, buffer, 1);
				if (done)
				{
					unsigned char status = buffer[0];
					if (((status & 0x70) >> 4) == 1) // Accessory signature successfully generated
						break;
					else done = 0;
				}
				exos_thread_sleep(1000);
			}

			done = _read(CP20_REG_ERROR_CODE, buffer, 1);

			if (done)
			{
				done = _read(CP20_REG_SIGNATURE_DATA_LENGTH, buffer, 2);
				if (done)
				{
					int sig_len = (buffer[0] << 8) | buffer[1];
					done = _read(CP20_REG_SIGNATURE_DATA, sig, sig_len);
					if (done)
						return sig_len;
				}
			}
		}
	}
	return 0;
}
