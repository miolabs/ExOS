#include "measure.h"
#include <support/misc/stc3105.h>

static int _vbat, _current, _charge;
#define VBAT_RATIO (2440 * 350 / 20)
#define VCURR_RATIO (11770 / 2)

void measure_initialize()
{
	int done = stc3105_initialize(0, STC3105_DEFAULT_ADDRESS, STC3105F_NONE);
}

void measure_update()
{
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
	}
}

int measure_vbat()
{
	return _vbat;
}

int measure_current()
{
	return _current;
}

int measure_charge()
{
	return _charge;
}
