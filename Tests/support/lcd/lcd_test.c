#include <support/lcd/lcd.h>
#include <modules/gfx/mono.h>
#include <kernel/thread.h>
#include <stdio.h>
#include <assert.h>
#include <kernel/timer.h>

#include "xkuty/xkuty_gfx.h"

#define DISPW 128
#define DISPH 64

static unsigned long _screen_pixels [(DISPW*DISPH)/32];

static const CANVAS _screen = 
{ 
	(unsigned char *)_screen_pixels,
    DISPW, DISPH,
    DISPW/8,
    PIX_1_MONOCHROME
};

static inline int _xkutybit(int x, int y, int t, int cx, int cy)
{
	int xx = x - cx; int yy = y - cy;
	int r = xx*xx + yy*yy;
	return ( r < t) ? 1 : 0;
}

void _xkuty_effect(const unsigned int* bitmap, int t, int cx, int cy)
{
	unsigned int* pixels = (unsigned int *)_screen.Pixels;
	if ( t<0) t=0x7fffffff;		// Show the full bitmap
    int i=0, x=0, y=0, sx=0;
    for (y=0; y<_screen.Height; y++)
	{
        for ( x=0; x<_screen.Width; x+=32)
        {
			unsigned int pic = bitmap [i];
        	int bits = _xkutybit ( x+0, y, t, cx, cy);
			for (sx=0; sx<32; sx++)
			{
				bits <<= 1;
				bits |= _xkutybit ( x+sx, y, t, cx, cy);
			}
			pixels [i] = bits & pic;
            i++;
        }
	}
}

enum
{
	ST_INTRO_SECTION = 1,
    ST_LOGO_IN,
    ST_LOGO_SHOW,
    ST_LOGO_OUT,
    ST_EXOS_IN,
    ST_EXOS_SHOW,
    ST_EXOS_OUT,
	ST_INTRO_SECTION_END,
};

static void _intro ( int* status, int* st_time_base, int time)
{
	int factor;
	const int cx = _screen.Width >> 1, cy=_screen.Height >> 1;
	const int logo_time = 600; // ms
	switch(*status)
	{
		case ST_LOGO_IN:
			factor = time/6;
			_xkuty_effect ( _xkuty2_bw, factor * factor, cx, cy);
			if ( factor > 75) 
				*status = ST_LOGO_SHOW, *st_time_base = time;
			break;
		case ST_LOGO_SHOW:
			_xkuty_effect ( _xkuty2_bw, -1, cx, cy);
			if ( time >= (*st_time_base+logo_time)) 
				*status = ST_LOGO_OUT, *st_time_base = time;
			break;
		case ST_LOGO_OUT:
			factor = 75-((time - *st_time_base) / 6);
			_xkuty_effect(_xkuty2_bw, factor * factor, cx, cy);
			if ( factor <= 0) 
				*status = ST_EXOS_IN, *st_time_base = time;
			break;
		case ST_EXOS_IN:
			factor = ((time - *st_time_base)/3);
			_xkuty_effect(_exos_bw, factor * factor, 0, DISPH);
			if ( factor > 140) 
				*status = ST_EXOS_SHOW, *st_time_base = time;
			break;
		case ST_EXOS_SHOW:
			_xkuty_effect(_exos_bw, -1, 0, DISPH);
			if ( time >= (*st_time_base+logo_time)) 
				*status = ST_EXOS_OUT, *st_time_base = time;
			break;
		case ST_EXOS_OUT:
			factor = 140 - ((time - *st_time_base)/3);
			_xkuty_effect(_exos_bw, factor * factor, _screen.Width, _screen.Height);
			if ( factor <= 0) 
				*status = ST_LOGO_IN;
			break;
	}
}

#define MAIN_LOOP_TIME  20

void main()
{
	lcd_initialize();
	lcdcon_gpo_backlight(1);

	unsigned int st_time_base = 0;
    unsigned int time_base = exos_timer_time();
	unsigned int prev_time = 0;

	int screen_count = 0;
	int status = ST_LOGO_IN;
    while (1)
    {
		unsigned int time = exos_timer_time();
		unsigned int req_update_time = MAIN_LOOP_TIME;
		time -= time_base;
		unsigned int elapsed_time = time - prev_time;
		int frame_skips = 0;

		_intro(&status, &st_time_base, time);

		// Screen conversion & dump
		if ( screen_count > frame_skips)
		{
			lcd_dump_screen(_screen.Pixels);
			screen_count=0;
//			_frame_dumps++;
		}
		screen_count++;

		if ( elapsed_time < req_update_time)
			exos_thread_sleep( req_update_time - elapsed_time);
		prev_time = time;
    }
}

