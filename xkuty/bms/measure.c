#include "measure.h"
#include <support/misc/stc3105.h>
#include <support/adc_hal.h>
#include <CMSIS/LPC11xx.h>

static int _vbat, _current, _soc, _vchg;
static unsigned short _vcell[CELL_COUNT];
static int _vbat_miss;

#define VBAT_RATIO (2440 * 350 / 20)
#define VCURR_RATIO (11770 / 2)
#define VSOC_RATIO (1)	// FIXME
#define VCELL_RATIO (3300 * 1.5195)
#define VIN_RATIO (3300 * 350 / 20)

int measure_initialize()
{
	int done = stc3105_initialize(0, STC3105_DEFAULT_ADDRESS, STC3105F_NONE);
	if (done)
	{
		// ADMUX control
		LPC_GPIO3->DIR |= 0xf;
		// ADEN
		LPC_GPIO2->DIR |= (1<<5);
		LPC_GPIO2->MASKED_ACCESS[1<<5] = 1<<5;

		hal_adc_initialize(10000, 12);
	}

	_vbat_miss = 0;
	return done;
}

MEASURE_ERROR measure_update()
{
	MEASURE_ERROR err = MEASURE_OK;
	unsigned short vbat;
	signed short curr, soc;

    STC3105_UPDATE update = stc3105_update(&vbat, &curr, &soc);
	if (update & STC3105_UPD_VOLTAGE)
	{
		_vbat = (vbat * VBAT_RATIO) / 1000;
	}
	if (update & STC3105_UPD_CURRENT)
	{
		_current = (curr * VCURR_RATIO) / 1000;
	}
	if (update & STC3105_UPD_CHARGE)
	{
		_soc = (soc * VSOC_RATIO) / 1000;
	}

	unsigned long total = 0;
	for(int inp = 0; inp < CELL_COUNT; inp++)
	{
		LPC_GPIO3->MASKED_ACCESS[0xF] = inp;
		total += _vcell[inp] = (hal_adc_read(1) * (unsigned long)VCELL_RATIO) >> 16;
	}

	unsigned long vin = (hal_adc_read(0) * (unsigned long)VIN_RATIO) >> 16;
	if (update & STC3105_UPD_VOLTAGE)
	{
		int diff = (vin > _vbat) ? (vin - _vbat) : (_vbat - vin);
		if (diff > 100)
			err = MEASURE_VIN_MISMATCH;

		_vbat_miss = 0;
	}
	else
	{
		if (_vbat == 0) _vbat = vin;
		if (++_vbat_miss > 10)
			err = MEASURE_SUPERVISOR_FAILURE; 
	}
	
	_vchg = (hal_adc_read(2) * (unsigned long)VIN_RATIO) >> 16;
	return err;
}

int measure_vbat()
{
	return _vbat;
}

int measure_current()
{
	return _current;
}

int measure_soc()
{
	return _soc;
}

int measure_vchg()
{
	return _vchg;
}

int measure_cell_balance(int *pcell)
{
	return 0;
}
