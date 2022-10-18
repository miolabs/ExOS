#include "cp30.h"
#include <support/i2c_hal.h>
#include <kernel/thread.h>
#include <kernel/verbose.h>

#ifdef DEBUG
#define _verbose(level, ...) verbose(level, "cp30", __VA_ARGS__)
#else
#define _verbose(...) { /* nothing */ }
#endif


static int _addr;

bool apple_cp30_initialize()
{
	unsigned long auth_id;
	
	for(int i = 0; i <= 1; i++)
	{
		_addr = 0x10 + i;
		bool done = apple_cp30_read_device_id(&auth_id);
		if (done && auth_id == 0x300) 
		{
			_verbose(VERBOSE_DEBUG, "Apple CP detected at i2c addr 0x%x", _addr);
			return true;
		}
	}
	_verbose(VERBOSE_ERROR, "Apple CP not detected!");
	return false;
}

static bool _read(cp30_reg_t reg, unsigned char *buffer, unsigned char length)
{
	i2c_context_t i2c;

#ifdef APPLE_CP30_I2C_MODULE
	bool done = false;
	for (int retry = 0; retry < 3; retry++)
	{
		hal_i2c_init_context(&i2c, _addr, NULL, 0);
		done = hal_i2c_write(APPLE_CP30_I2C_MODULE, &i2c, &reg, 1);
		if (done)
		{
			for (int retry2 = 0; retry2 < 10; retry2++)
			{
				exos_thread_sleep(1);
				hal_i2c_init_context(&i2c, _addr, NULL, 0);
				done = hal_i2c_read(APPLE_CP30_I2C_MODULE, &i2c, buffer, length);
				if (done) return true;
			}
		}
		exos_thread_sleep(1);
	}
#elif DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return false;
}

static bool _write(cp30_reg_t reg, unsigned char *buffer, unsigned char length)
{
	i2c_context_t i2c;

#ifdef APPLE_CP30_I2C_MODULE
	bool done;
	for (int retry = 0; retry < 10; retry++)
	{
		hal_i2c_init_context(&i2c, _addr, &reg, 1);		
		done = hal_i2c_write(APPLE_CP30_I2C_MODULE, &i2c, buffer, length); 
		if (done) return true;
		exos_thread_sleep(1);
	}
#elif DEBUG
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
#endif
	return false;
}

bool apple_cp30_read_device_id(unsigned long *pdevice_id)
{
	int done;
    unsigned char buffer[4];
	done = _read(CP20_REG_DEVICE_ID, buffer, 4);
	if (done)
	{
		*pdevice_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];

#ifdef DEBUG
		//done = _read(CP20_REG_ERROR_CODE, buffer, 1);
		//buffer[0] = 0x01;
		//done = _write(CP20_REG_SELFTEST_CONTROL_AND_STATUS, buffer, 1);
		//exos_thread_sleep(10);
		//done = _read(CP20_REG_SELFTEST_CONTROL_AND_STATUS, buffer, 1);
		//ASSERT(done && ((buffer[0] & 0xf0) == 0xc0), KERNEL_ERROR_KERNEL_PANIC);
#endif
	}
	return done;
}

#if 0
bool apple_cp2_read_acc_cert_length(unsigned short *plength)
{
    unsigned char buffer[2];
	bool done = _read(CP20_REG_ACCESORY_CERTIFICATE_DATA_LENGTH, buffer, 2);
	if (done)
	{
		*plength = (buffer[0] << 8) | buffer[1];
		return true;
	}
	return false;
}

bool apple_cp2_read_acc_cert_page(int page, unsigned char *buffer, int length)
{
	bool done = _read(CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE1 + page, buffer, length);
	return done;
}

int apple_cp2_get_auth_signature(unsigned char *challenge, int ch_len, unsigned char *sig)
{
	bool done;
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
					else 
					{
						done = _read(CP20_REG_ERROR_CODE, buffer, 1);
						_verbose(VERBOSE_ERROR, "Signature generation failed (status=$%02x, error=$%02x)!", 
							status, buffer[0]);
						done = 0;
					}
				}
				exos_thread_sleep(1000);
			}

			if (done)
			{
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
					else _verbose(VERBOSE_ERROR, "Could not read signature!");
				}
			}
		}
		else _verbose(VERBOSE_ERROR, "Could not write challenge!");
	}
	_verbose(VERBOSE_ERROR, "Signature not done!");
	return 0;
}
#endif

