// LPC17xx I2C Peripheral Support
// by Miguel Fides

#include "i2c.h"
#include "cpu.h"
#include <support/i2c_hal.h>
#include <support/board_hal.h>

#define I2C_MODULE_COUNT 3

static I2C_MODULE *_modules[] = { 
	(I2C_MODULE *)LPC_I2C0_BASE, 
	(I2C_MODULE *)LPC_I2C1_BASE, 
	(I2C_MODULE *)LPC_I2C2_BASE };

static inline I2C_MODULE *_get_module(int module)
{
	return (module < I2C_MODULE_COUNT) ? _modules[module] : (void *)0;
}

void hal_i2c_initialize(int module, int bitrate)
{
	int pclk;
	I2C_MODULE *i2c = _get_module(module);
	switch(module)
	{
		case 0:
			LPC_SC->PCONP |= PCONP_PCI2C0;
			pclk = cpu_pclk(PCLK_I2C0);
			break;
		case 1:
			LPC_SC->PCONP |= PCONP_PCI2C1;
			pclk = cpu_pclk(PCLK_I2C1);
			break;
		case 2:
			LPC_SC->PCONP |= PCONP_PCI2C2;
			pclk = cpu_pclk(PCLK_I2C2);
			break;
		default:
			return;
	}

	if (i2c)
	{
		i2c->I2CONCLR = 0xFF;
		int third_divider = pclk / (bitrate * 3);
		i2c->I2SCLH = third_divider;
		i2c->I2SCLL = third_divider * 2;
		i2c->I2CONSET = I2C_CON_I2EN;	// enable module
	}
}

static int _wait_si(I2C_MODULE *i2c)
{
	int result = 0;
	for(int loops = 0; loops < 1000; loops++)
	{
		if (i2c->I2CONSET & I2C_CON_SI) 
		{ 
			result = 1; 
			break;
		}
	}
	return result;
}

int hal_i2c_master_frame(int module, unsigned char slave, 
	unsigned char *buffer, int write_len, int read_len)
{
	I2C_MODULE *i2c = _get_module(module);
	if (i2c == (I2C_MODULE *)0) return -1;

	i2c->I2CONCLR = I2C_CON_SI;	
	int error = 0;
	int pos = 0;

	// send Start condition
	i2c->I2CONSET = I2C_CON_START;
	if (!_wait_si(i2c)) error = 1;
	else if ((i2c->I2STAT != 0x08) && (i2c->I2STAT != 0x10)) error = 2;

	if (write_len != 0 && error == 0)
	{
		i2c->I2DAT = slave << 1;	// SLA + W (low bit = 0)
		i2c->I2CONSET = I2C_CON_AA;
		i2c->I2CONCLR = I2C_CON_START | I2C_CON_SI;
		if (!_wait_si(i2c)) error = 1;
		else if (i2c->I2STAT != 0x18) error = 3;
		
		while(write_len-- && error == 0)
		{
			i2c->I2DAT = buffer[pos++];
            i2c->I2CONCLR = I2C_CON_SI;	
			if (!_wait_si(i2c)) error = 1;
			else if (i2c->I2STAT != 0x28) error = 4;
		}

		if (read_len != 0 && error == 0)
		{
			// send Repeated Start to switch to continue
			i2c->I2CONSET = I2C_CON_START;
            i2c->I2CONCLR = I2C_CON_SI;	
			if (!_wait_si(i2c)) error = 1;
			else if (i2c->I2STAT != 0x10) error = 5;
		}
	}

	if (read_len != 0 && error == 0)
	{
		i2c->I2DAT = (slave << 1) | 1 ;	// SLA + R (low bit = 1)
		i2c->I2CONSET = I2C_CON_AA;
		i2c->I2CONCLR = I2C_CON_START | I2C_CON_SI;
		if (!_wait_si(i2c)) error = 1;
		else if (i2c->I2STAT != 0x40) error = 6;

		while (read_len-- && error == 0)
		{
			if (read_len != 0)
			{
				i2c->I2CONCLR = I2C_CON_SI;
				if (!_wait_si(i2c)) error = 1;
				else if (i2c->I2STAT != 0x50) error = 7;
				else buffer[pos++] = i2c->I2DAT;
			}
			else
			{
				i2c->I2CONCLR = I2C_CON_SI | I2C_CON_AA;
				if (!_wait_si(i2c)) error = 1;
				else if (i2c->I2STAT != 0x58) error = 8;
				else buffer[pos++] = i2c->I2DAT;
			}
		}
	}

	if (error) 
	{
		i2c->I2CONCLR = I2C_CON_START;
	}
	i2c->I2CONSET = I2C_CON_STOP;
    i2c->I2CONCLR = I2C_CON_SI;
	return error;
}


