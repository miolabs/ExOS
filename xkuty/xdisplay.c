#include "xdisplay.h"
#include "xcpu.h"
#include "xanalog.h"
#include "event_recording.h"
#include <support/lcd/lcd.h>
#include <stdio.h>
#include <assert.h>

#include <modules/gfx/mono.h>
#include <modules/gfx/font.h>
#include "xkuty_gfx.h"
#include "small_font.h"

#define DISPW 128
#define DISPH 64

static unsigned int _screen_pixels[(DISPW*DISPH)/32];

static const CANVAS _screen = 
{ 
	(unsigned char *)_screen_pixels,
    DISPW, DISPH,
    DISPW/8,
    PIX_1_MONOCHROME
};

static void _bilevel_linear_2_lcd(unsigned char *dst, unsigned int *src, int w, int h);

static void _clean_bilevel(const CANVAS *scr)  
{ 
	unsigned int* pix = (unsigned int*)scr->Pixels;
	int words = (scr->StrideBytes * scr->Height) >> 2;
	for (int i=0; i<words; i++) 
		pix [i] = 0;  
}

static inline int _xkutybit(int x, int y, int t, int cx, int cy)
{
	int xx = x - cx; int yy = y - cy;
	int r = xx*xx + yy*yy;
	return ( r < t) ? 1 : 0;
}

void _xkuty_effect(const unsigned int *bitmap, int t, int cx, int cy)
{
	unsigned int *pixels = (unsigned int *)_screen.Pixels;
	if (t<0) t=0x7fffffff;		// Show the full bitmap
    int i=0, x=0, y=0, sx=0;
    for (y=0; y<_screen.Height; y++)
	{
        for ( x=0; x<_screen.Width; x+=32)
        {
			unsigned int pic = bitmap[i];
        	int bits = _xkutybit(x+0, y, t, cx, cy);
			for (sx=0; sx<32; sx++)
			{
				bits <<= 1;
				bits |= _xkutybit(x+sx, y, t, cx, cy);
			}
			pixels [i] = bits & pic;
			i++;
        }
	}
}

static void _draw_text ( const char* text, const SPRITE* font, int x, int y)
{ 
	SPRITE spr = *font;
	int i = 0;
	while ( text[i])
	{
		int glyph_w = font->Width;
		int glyph = -1;
		if (( text [i]>='0' && text [i]<='9'))
			glyph = text [i] - '0';
		else
			switch ( text [i])	// Hyper-hack
			{
				case '.': glyph = 10, glyph_w = (font->Width >> 1)+1; break;
				case '-': glyph = 11; break;
			}
		if ( glyph != -1)
		{
			spr.Bitmap = font->Bitmap + (spr.BitmapStride * spr.Height * glyph);
			mono_draw_sprite ( &_screen, &spr, x, y);
		}
		x += glyph_w + 1;
		i++;
	}
}


static void _vertical_sprite_comb ( const SPRITE* spr0, const SPRITE* spr1, 
                                    int level_fx8, int x, int y)
{
	SPRITE spr = *spr1;
	level_fx8 = __LIMIT ( level_fx8, 0,0x100);
	int cut_y = (level_fx8 * spr.Height) >> 8;
	spr.Height = cut_y;
	if (spr.Height > 0)
		mono_draw_sprite(&_screen, &spr, x, y);

	spr = *spr0;
	spr.Bitmap = spr0->Bitmap + (spr.BitmapStride * cut_y);
	spr.Height = spr.Height - cut_y;
	if (spr.Height > 0)
		mono_draw_sprite(&_screen, &spr, x, y + cut_y);
}

static void _horizontal_sprite_comb(const SPRITE* spr0, const SPRITE* spr1, int level_fx8, int x, int y)
{
	static unsigned int show_mask[] = {0,0,0,0};

	mono_draw_sprite(&_screen, spr1, x, y);

	level_fx8 = __LIMIT(level_fx8, 0, 0x100);
	int cut_x = (level_fx8 * spr0->Width) >> 8;
	int spans = cut_x >> 5;
	int inter = cut_x & 0x1f;
	int i = 0;
	for ( ; i < 4; i++)
		if (i < (cut_x >> 5)) 
			show_mask[i] = 0xffffffff;
		else
			if ((cut_x >> 5) == i)
				show_mask[i] = (unsigned int)(-1 << (32 - inter));
			else
				show_mask[i] = 0;
	SPRITE spr = *spr0;
	spr.Mask = show_mask;
	spr.MaskStride = 0;
	mono_draw_sprite(&_screen, &spr, x, y);
}

static void _print_small(const char *str, int x, int y)
{
	if (x == -1)
	{
		int t = font_calc_len(&font_small, str, FONT_PROPORTIONAL | FONT_KERNING);
		x = 64 - (t/2);
	}
	font_draw(&_screen, str, &font_small, FONT_PROPORTIONAL | FONT_KERNING, x, y);
}

static int _frame_dumps = 0;

void xdisplay_initialize()
{
	lcd_initialize();
}

void xdisplay_clean_screen()
{
	_clean_bilevel(&_screen);
}

void xdisplay_dump()
{
	lcd_dump_screen(_screen.Pixels);
	_frame_dumps++;
}

void xdisplay_intro(DISPLAY_STATE *status, int *st_time_base, int time)
{
	int factor;
	const int cx = _screen.Width >> 1, cy = _screen.Height >> 1;
	const int logo_time = 600; // ms
	switch(*status)
	{
		case ST_LOGO_IN:
			factor = time/6;
			_xkuty_effect(_xkuty2_bw, factor * factor, cx, cy);
			if (factor > 75) 
				*status = ST_LOGO_SHOW, *st_time_base = time;
			break;
		case ST_LOGO_SHOW:
			_xkuty_effect(_xkuty2_bw, -1, cx, cy);
			if (time >= (*st_time_base+logo_time)) 
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
			if (factor > 140) 
				*status = ST_EXOS_SHOW, *st_time_base = time;
			break;
		case ST_EXOS_SHOW:
			_xkuty_effect(_exos_bw, -1, 0, DISPH);
			if (time >= (*st_time_base + logo_time)) 
				*status = ST_EXOS_OUT, *st_time_base = time;
			break;
		case ST_EXOS_OUT:
			factor = 140-((time - *st_time_base)/3);
			_xkuty_effect(_exos_bw, factor * factor, _screen.Width, _screen.Height);
			if (factor <= 0) 
				*status = ST_DASH;
			break;
	}
}

#define POS_BATTERY_BAR   5,   4
#define POS_NEUTRAL       98,  7
#define POS_FATAL         80,  12
#define POS_SPEED_TEXT    124, 9
#define POS_DISTANCE_TEXT 124, 50
#define POS_CRUISE        36,  8
#define POS_WARNING       37,  28
#define POS_KMH           100, 4
#define POS_KM            108, 41
#define POS_MI            102, 37

#define POS_ADJUST_MSG    16, 2
#define POS_ADJUST_BAR    40, 46
#define POS_ADJUST_SPEED  36, 20

//static void _adjust_screen (DISPLAY_STATE status, char *str, int next, unsigned short *res)
static void _adjust_screen(DISPLAY_STATE state, char *str, unsigned char actual)
{
	unsigned char tmp[20];
	_print_small(str, -1, 14);
	sprintf(tmp, "%d", actual);
	_draw_text(tmp, &_font_spr_small, 48, 26);

	_horizontal_sprite_comb(&_adjust_full_spr, &_adjust_empty_spr, actual, POS_ADJUST_BAR);
}

extern char _adj_drive_mode;

void xdisplay_runtime_screens(DISPLAY_STATE state, DASH_DATA *dash)
{
	unsigned char tmp[20];
	
	switch(state)
	{
		case ST_DASH:
			{
				//_dash.status |= XCPU_STATE_CRUISE_ON;
				//_dash.status |= XCPU_STATE_WARNING;
                //_dash.status |= XCPU_STATE_ERROR;
				int l = sprintf(tmp, "%d", dash->Speed);
				if (dash->CpuStatus & (XCPU_STATE_NEUTRAL | XCPU_STATE_ERROR))
				{
					if (dash->CpuStatus & XCPU_STATE_NEUTRAL)
						mono_draw_sprite(&_screen, &_lock_spr, POS_NEUTRAL);
					if ((dash->CpuStatus & XCPU_STATE_ERROR) && ((_frame_dumps & 8) == 0))
						mono_draw_sprite(&_screen, &_fatal_error_spr, POS_FATAL);
				}
				else
					_draw_text(tmp, &_font_spr_big, - (24*l) + POS_SPEED_TEXT);

				l = sprintf(tmp, "%d.%d", dash->Distance / 10, dash->Distance % 10);
				_draw_text(tmp, &_font_spr_small, - (_font_spr_small.Width * l) + POS_DISTANCE_TEXT);

                _vertical_sprite_comb(&_battery_full, &_battery_empty, 0x100 - dash->battery_level_fx8, POS_BATTERY_BAR);

				if (dash->CpuStatus & XCPU_STATE_CRUISE_ON)
					mono_draw_sprite(&_screen, &_cruisin_spr, POS_CRUISE);
				if (dash->CpuStatus & XCPU_STATE_WARNING)
					mono_draw_sprite(&_screen, &_warning_spr, POS_WARNING);

				//mono_draw_sprite ( &screen, &_kmh_spr, POS_KMH);
                if (dash->CpuStatus & XCPU_STATE_MILES)
					mono_draw_sprite(&_screen, &_mi_spr, POS_MI);
				else
					mono_draw_sprite(&_screen, &_km_spr, POS_KM);
			}
			break;

		case ST_DEBUG_INPUT:
			{
				int vv[6];
				char* sensor_names[] = {"Throttle", "R Brake", "F Brake", "Cruise", "Horn"};
				for (int i = 0; i < ANALOG_INPUT_COUNT; i++)
				{
					int y = 10 + 12*i;
					ANALOG_INPUT *ain = xanalog_input(i);
					int vv[] = { ain->Current, ain->Filtered, ain->Scaled, ain->Min, ain->Max, 0 };
#ifdef DEBUG
					vv[5] = ain->Fir.discarded;
#endif
					_print_small(sensor_names[i], 0, y);
					for (int j = 0; j < (sizeof(vv) / sizeof(*vv)); j++)
					{
						sprintf(tmp, "%d%%", (vv[j] * 100) >> 12);
						_print_small(tmp, 36 + (j * 16), y);
					}
				}
			}
			break;
		case ST_ADJUST_WHEEL_DIA:
			{
				dash->ActiveConfig.SpeedAdjust = __LIMIT(dash->ActiveConfig.SpeedAdjust, -10, 10);
				int bar = 0x80 + (dash->ActiveConfig.SpeedAdjust * (0x80 / 10));
				mono_draw_sprite ( &_screen, &_speed_adjust_spr, POS_ADJUST_MSG);
				sprintf(tmp, "%d", dash->ActiveConfig.SpeedAdjust);
				_draw_text(tmp, &_font_spr_big, POS_ADJUST_SPEED);
									
				_horizontal_sprite_comb ( &_adjust_full_spr, &_adjust_empty_spr, bar, POS_ADJUST_BAR); 
			}
			break;
		case ST_ADJUST_THROTTLE_MAX:
			_adjust_screen(state, "Push max throttle", dash->ActiveConfig.ThrottleMax);
			break;		
		case ST_ADJUST_THROTTLE_MIN:
			_adjust_screen(state, "Release throttle", dash->ActiveConfig.ThrottleMin);
			break;

		case ST_FACTORY_MENU:
			{
				const char _hei [] = { 22, 31, 40, 49, 58 };
				const EVREC_CHECK fact_menu_exit[] = {{HORN_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
				const EVREC_CHECK menu_move[] = {{BRAKE_FRONT_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
				const EVREC_CHECK menu_press[] = {{CRUISE_MASK, CHECK_RELEASE}, {0x00000000, CHECK_END}};
				int anm = (_frame_dumps & 0x7) >> 2;
				_print_small("OEM SETTINGS", -1,  10);
				if (dash->CpuStatus & XCPU_STATE_MILES)
					_print_small("Km/Mi: Miles", -1, _hei[0]);
				else
					_print_small("Km/Mi: Km", -1, _hei[0]);

				_print_small("Wheel const.", -1, _hei[1]);
				_print_small("Throttle adjust", -1, _hei[2]);
				_print_small("Switch lights", -1, _hei[3]);
				_print_small("Sensor monitor", -1, _hei[4]);
				if (_frame_dumps & 0x8)
					_print_small(">>", 20, _hei[dash->CurrentMenuOption] - 1);
			}
			break;

		case ST_ADJUST_DRIVE_MODE:
			{
				const char _hei[] = { 30, 45, 60 };
				const EVREC_CHECK speed_adj_exit[]= { { CRUISE_MASK, CHECK_RELEASE }, { 0x00000000, CHECK_END } };
				const EVREC_CHECK mode_adj[]= { { BRAKE_FRONT_MASK, CHECK_RELEASE }, { 0x00000000, CHECK_END } };
				int anm = (_frame_dumps & 0x7) >> 2;
				_print_small("DRIVE MODES", -1, 14);
				_print_small("Soft", -1, _hei[0]);
				_print_small("Eco", -1, _hei[1]);
				_print_small("Racing", -1, _hei[2]);
                if (_frame_dumps & 0x8)
					_print_small(">>", 24, _hei[_adj_drive_mode]);
			}
			break;
	}
}






