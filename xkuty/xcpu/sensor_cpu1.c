#include "sensor_cpu1.h"
#include <support/adc_hal.h>

#define BATT_VOLTAGE_RATIO  16.9f

void xcpu_sensor_initialize()
{
	hal_adc_initialize(1000, 16);
}

int xcpu_sensor_batt_level()
{
	int batt = hal_adc_read(1) >> 6;	// Battery voltage = Input adc / BATT_VOLTAGE_RATIO
	const int bot = (int)(3.5f * 13.0f * BATT_VOLTAGE_RATIO);	// Discharged level 3.5v
	const int top = (int)(4.0f * 13.0f * BATT_VOLTAGE_RATIO); // Charged value 4.0v
	batt = ((batt - bot) * (0x10000 / (top - bot))) >> 8;
	if (batt < 0) batt = 0;
	if (batt > 0xff) batt = 0xff;
	return batt;
}
