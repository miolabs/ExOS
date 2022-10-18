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

static bool _send_byte(unsigned char c)
{
	for (unsigned i = 0; i < 8; i++)
	{
		soft_i2c_assert_clk(c & 0x80);
		c <<= 1;
		_delay();
		soft_i2c_release_clk();
		_delay();
	}

	soft_i2c_assert_clk(1);
	_delay();
	soft_i2c_release_clk();
	_delay();

	bool nack;
	for(unsigned i = 0; i < SOFT_MAX_CLOCK_STRETCH; i++)
		if (soft_i2c_read_data(&nack)) break;
		else _delay();
		
	return !nack;
}

static bool _send_seq(i2c_context_t *context, const unsigned char *buf, unsigned length)
{
	for(unsigned i = 0; i  < length; i++)
	{
		if (!_send_byte(buf[i]))
		{
			context->Error = I2C_NACK;
			return false;
		}
	}
	return true;
}

static bool _read_byte(unsigned char *pc, bool ack)
{
	unsigned char c = 0;
	for (unsigned i = 0; i < 8; i++)
	{
		soft_i2c_assert_clk(1);
		_delay();
		soft_i2c_release_clk();
		_delay();
		
		bool bit;
		for(unsigned i = 0; i < SOFT_MAX_CLOCK_STRETCH; i++)
			if (soft_i2c_read_data(&bit)) break;
			else _delay();
		
		c = (c << 1) | (bit & 1);
 	}

	soft_i2c_assert_clk(ack ? 0 : 1);
	_delay();
	soft_i2c_release_clk();
	_delay();

	*pc = c;

	bool nack;
	for(unsigned i = 0; i < SOFT_MAX_CLOCK_STRETCH; i++)
		if (soft_i2c_read_data(&nack)) return true;
		else _delay();
		
	return false;	
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

void hal_i2c_initialize(unsigned module, unsigned bitrate)
{
	// TODO
}

bool hal_i2c_write(unsigned module, i2c_context_t *context, const void *data, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	if (soft_i2c_start(true))
	{
		_delay();
		context->Error = I2C_OK;

		if (_send_byte(context->Address << 1))
		{
			do
			{
				if (context->CmdLength != 0)
				{
					if (_send_seq(context, context->Cmd, context->CmdLength))
					{
						context->CmdLength = 0;
						continue;
					}
				}
				else if (length != 0)
				{
					_send_seq(context, data, length);
				}
			} while(0);
		}
		else context->Error = I2C_NACK;

		soft_i2c_assert_clk(0);
		_delay();
		soft_i2c_release_clk();
		_delay();

		soft_i2c_start(false);
	}
	else context->Error = I2C_BUS_ERROR;

	return context->Error == I2C_OK;
}

bool hal_i2c_read(unsigned module, i2c_context_t *context, void *data, unsigned length)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	if (soft_i2c_start(true))
	{
		_delay();
		context->Error = I2C_OK;

		if (_send_byte((context->Address << 1) | 1))
		{
			do
			{
				ASSERT(context->CmdLength == 0, KERNEL_ERROR_NOT_IMPLEMENTED);
				if (context->CmdLength != 0)
				{
					if (_send_seq(context, context->Cmd, context->CmdLength))
					{
						context->CmdLength = 0;
						continue;
					}
				}
				else if (length != 0)
				{
					_read_seq(context, data, length);
				}
			} while(0);
		}
		else context->Error = I2C_NACK;

		soft_i2c_assert_clk(0);
		_delay();
		soft_i2c_release_clk();
		_delay();

		soft_i2c_start(false);
	}
	else context->Error = I2C_BUS_ERROR;	

	return context->Error == I2C_OK;
}



