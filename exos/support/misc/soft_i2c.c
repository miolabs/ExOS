#include "soft_i2c.h"
#include <support/i2c_hal.h>
#include <kernel/panic.h>

#ifndef SOFT_I2C_DELAY
#define SOFT_I2C_DELAY 22
#endif

#define SOFT_MAX_CLOCK_STRETCH 1000

static void _delay()
{
	for(unsigned volatile i = 0; i  < SOFT_I2C_DELAY; i++);
}

static bool _read_bit(bool out, bool *pin)
{
	soft_i2c_assert_clk(out);
	_delay();
	soft_i2c_release_clk();
	_delay();

	bool bit;
	for(unsigned i = 0; i < SOFT_MAX_CLOCK_STRETCH; i++)
	{
		if (soft_i2c_read_data(&bit)) 
		{
			if (pin != NULL) *pin = bit;
			return true;
		}
		else _delay();
	}
	return false;
}

static bool _send_byte(i2c_context_t *context, unsigned char c)
{
	bool input;
	for (unsigned i = 0; i < 8; i++)
	{
		if (!_read_bit(0 != (c & 0x80), &input))
		{
			context->Error = I2C_BUS_ERROR;
			return false;
		}

		c <<= 1;
	}

	if (!_read_bit(1, &input))
	{
		context->Error = I2C_BUS_ERROR;
		return false;
	}
	if (input != 0)
	{
		context->Error = I2C_NACK;
		return false;
	}
	return true;
}

static bool _send_seq(i2c_context_t *context, const unsigned char *buf, unsigned length)
{
	for(unsigned i = 0; i  < length; i++)
	{
		if (!_send_byte(context, buf[i]))
			return false;
	}
	return true;
}


static bool _read_byte(unsigned char *pc, bool ack)
{
	unsigned char c = 0;
	for (unsigned i = 0; i < 8; i++)
	{
		bool bit;
		if (!_read_bit(1, &bit))
			return false;
		c <<= 1;
		if (bit) c |= 1;
 	}

	if (!_read_bit(ack ? 0 : 1, NULL))
		return false;

	*pc = c;
	return true;
}

static bool _read_seq(i2c_context_t *context, unsigned char *buf, unsigned length)
{
	for(unsigned i = 0; i  < length; i++)
	{
		if (!_read_byte(&buf[i], i != length - 1))
		{
			context->Error = I2C_BUS_ERROR;
			return false;
		}
	}
	return true;
}

static bool _stop(i2c_context_t *context)
{
	if (!_read_bit(0, NULL))
	{
		context->Error = I2C_BUS_ERROR;
		return false;
	}
	soft_i2c_start(false);	// NOTE: raise SDA
	return true;
}

void hal_i2c_initialize(unsigned module, unsigned bitrate)
{
	// NOTE: currently nothing
}

bool hal_i2c_write(unsigned module, i2c_context_t *context, const void *data, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	if (soft_i2c_start(true))
	{
		_delay();
		context->Error = I2C_OK;

		if (_send_byte(context, context->Address << 1))
		{
			while(1)
			{
				if (context->CmdLength != 0)
				{
					if (_send_seq(context, context->Cmd, context->CmdLength))
					{
						context->CmdLength = 0;
					}
				}
				else if (length != 0)
				{
					_send_seq(context, data, length);
					break;
				}
				else break;
			} 
		}
		
		_stop(context);
	}
	else context->Error = I2C_BUS_ERROR;

	return context->Error == I2C_OK;
}

bool hal_i2c_read(unsigned module, i2c_context_t *context, void *data, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	// TODO: write cmd before reading
	ASSERT(context->CmdLength == 0, KERNEL_ERROR_NOT_IMPLEMENTED);

	if (soft_i2c_start(true))
	{
		_delay();
		context->Error = I2C_OK;

		if (_send_byte(context, (context->Address << 1) | 1))
		{
			if (length != 0)
			{
				_read_seq(context, data, length);
			}
		}

		_stop(context);
	}
	else context->Error = I2C_BUS_ERROR;	

	return context->Error == I2C_OK;
}



