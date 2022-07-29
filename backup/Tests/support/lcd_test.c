#include <support/lcd/lcd.h>
#include <modules/gfx/mono.h>
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

