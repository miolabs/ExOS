#include "temperature.h"
#include <support/adc_hal.h>

void temp_initialize()
{
	hal_adc_initialize(10000, 12);
}

float temp_read()
{
	unsigned short value = hal_adc_read(0);
	float v = value * 3.3F / 65536;
	float temp = (v * 100) - 5;	 // LM35 linear scale
	return temp;
}


