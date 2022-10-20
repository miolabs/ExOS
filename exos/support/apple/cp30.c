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
	done = _read(CP30_REG_DEVICE_ID, buffer, 4);
	if (done)
	{
		*pdevice_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
	}
	return done;
}

bool apple_cp30_read_acc_cert_length(unsigned short *plength)
{
    unsigned char buffer[2];
	bool done = _read(CP30_REG_ACCESORY_CERTIFICATE_DATA_LENGTH, buffer, 2);
	if (done)
	{
		*plength = (buffer[0] << 8) | buffer[1];
		return true;
	}
	return false;
}

bool apple_cp30_read_acc_cert(unsigned char *buffer, unsigned short length)
{
	unsigned reg = CP30_REG_ACCESORY_CERTIFICATE_DATA1;
	unsigned short done = 0;
	while(done < length)
	{
		unsigned rem = length - done;
		if (rem > 128) rem = 128;
		if (_read(reg, buffer + done, rem))
		{
			done += rem;
			reg ++;
		}
		else break;
	}
	return done == length;
}

bool apple_cp30_begin_challenge(unsigned char *challenge, unsigned short length, unsigned short *presp_len)
{
	if (length != 32)
	{
		_verbose(VERBOSE_ERROR, "bad challenge data length!");
		return false;
	}

	unsigned char buffer[2];
	if (_write(CP30_REG_CHALLENGE_DATA, challenge, length)
		&& _read(CP30_REG_CHALLENGE_DATA_LENGTH, buffer, sizeof(buffer))
		&& buffer[0] == 0 && buffer[1] == 32)
	{
		bool done = false;
		unsigned char acas = CP30_ACAS_START;
		_write(CP30_REG_AUTH_CONTROL_AND_STATUS, &acas, 1);
		exos_thread_sleep(1);
		while(1)
		{
			if (_read(CP30_REG_AUTH_CONTROL_AND_STATUS, &acas, 1))
			{
				if (0 == (acas & CP30_ACAS_ERROR))
				{
					if (1 == ((acas & CP30_ACAS_RESULT_MASK) >> CP30_ACAS_RESULT_BIT))
					{
						done = true;
						break;
					}
				}
				else
				{
					// TODO read error code
					break;
				}
			}
			else break;

			break; // FIXME
		}

		if (done)
		{
			unsigned char buffer[2];
			if (_read(CP30_REG_CHALLENGE_RESP_DATA_LENGTH, buffer, sizeof(buffer)))
			{
				*presp_len = (buffer[1] << 8) | buffer[0];
				return true;
			}
		}
	}
	
	return false;
}

bool apple_cp30_read_challenge_resp(unsigned char *data, unsigned short length)
{
	bool done = _read(CP30_REG_CHALLENGE_RESP_DATA, data, length);
	return done;
}

