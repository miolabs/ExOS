#include <support/lcd/lcd.h>
#include <support/adc_hal.h>
#include <kernel/thread.h>
#include "kernel/timer.h"

void main()
{
	lcd_initialize();
	lcdcon_gpo_backlight(1);

	hal_adc_initialize(1000, 16);
	while(1)
	{
		unsigned short ain[6];
		for(int i = 0; i < 6; i++)
			ain[i] = hal_adc_read(i);

		// TODO: show inputs
	}
}

