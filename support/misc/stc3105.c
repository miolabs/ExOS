#include "stc3105.h"
#include <support/i2c_hal.h>

static unsigned char _module;
static unsigned char _address;

int stc3105_initialize(int module, int i2c_addr, STC3105_FLAGS config)
{
	unsigned char buffer[3];
	_module = module;
	_address = i2c_addr;

	buffer[0] = STC3105_REG_MODE;
	int err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
	if (err == 0)
	{
		unsigned char mode = buffer[1];
		unsigned char ctrl = buffer[2];

		mode |= STC3105_MODE_GG_RUN;
		if (config & STC3105F_POWER_SAVE_MODE)
			mode |= STC3105_MODE_PWR_SAVE;
		
		if (ctrl & STC3105_CTRL_PORDET)
		{
			ctrl |= STC3105_CTRL_GG_RST;	// reset charge accu and conversion counter
		}
		ctrl &= ~(STC3105_CTRL_PORDET 
			| STC3105_CTRL_GG_EOC | STC3105_CTRL_VM_EOC
			| STC3105_CTRL_ALM_SOC | STC3105_CTRL_ALM_VOLT);
		ctrl |= STC3105_CTRL_IO0DATA;

		buffer[0] = STC3105_REG_CTRL;
		buffer[1] = ctrl;
		err = hal_i2c_master_frame(_module, _address, buffer, 2, 0);
		if (err != 0) return 0;

		buffer[0] = STC3105_REG_MODE;
		buffer[1] = mode;
		err = hal_i2c_master_frame(_module, _address, buffer, 2, 0);
		if (err != 0) return 0;

		err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
		if (err == 0)
		{
			mode = buffer[1];
			return (mode & STC3105_MODE_GG_RUN) ? 1 : 0;
		}
	}
	return 0;
}

STC3105_UPDATE stc3105_update(unsigned short *pvbat, unsigned short *pcurr, unsigned short *psoc)
{
	unsigned char buffer[3];
	STC3105_UPDATE done = STC3105_UPD_NONE;

#ifdef DEBUG
	buffer[0] = STC3105_REG_MODE;
	int err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
	STC3105_CTRL ctrl = buffer[2];
#else
	buffer[0] = STC3105_REG_CTRL;
	int err = hal_i2c_master_frame(_module, _address, buffer, 1, 1);
	STC3105_CTRL ctrl = buffer[1];
#endif
	if (err == 0)
	{
		if (pvbat &&
			(ctrl & STC3105_CTRL_VM_EOC))
		{
			buffer[0] = STC3105_REG_VOLTAGE_LOW;
			err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
			if (err == 0)
			{
				*pvbat = buffer[1] | (buffer[2] << 8);
				done |= STC3105_UPD_VOLTAGE;
			}
		}

		if (ctrl & STC3105_CTRL_GG_EOC)
		{
			if (pcurr)
			{
				buffer[0] = STC3105_REG_CURRENT_LOW;
				err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
				if (err == 0)
				{
					*pcurr = buffer[1] | (buffer[2] << 8);
					done |= STC3105_UPD_CURRENT;
				}
			}
			if (psoc)
			{
				buffer[0] = STC3105_REG_CHARGE_LOW;
				err = hal_i2c_master_frame(_module, _address, buffer, 1, 2);
				if (err == 0)
				{
					*psoc = buffer[1] | (buffer[2] << 8);
					done |= STC3105_UPD_CHARGE;
				}
			}
		}
	}
	return done;
}
