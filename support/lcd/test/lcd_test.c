#include <support/lcd/lcd.h>
#include <support/lcd/mono.h>
#include <kernel/thread.h>
#include <stdio.h>
#include <assert.h>
#include <kernel/timer.h>

void main()
{
	lcd_initialize();
	lcdcon_gpo_backlight(1);

    while (1)
    {
    }
}

