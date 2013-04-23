
#include <support/lcd/lcd.h>
#include <support/lcd/mono.h>
#include <kernel/thread.h>
#include <support/adc_hal.h>
#include <stdio.h>
#include <assert.h>
#include "kernel/timer.h"

//void lcd_dump_screen ( char* pixels);

static unsigned char scrrot [(128*64)/8];
static unsigned int  screen [(128*64)/32];

static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h);

enum
{
    ST_LOGO_IN = 1,
    ST_LOGO_SHOW,
    ST_LOGO_OUT,
    ST_DASH
};

static const unsigned int xkuty_bw [];
const static unsigned int _bmp_battery [];
const static unsigned int _bmp_battery_empty [];
const static unsigned int _bmp_nums_distance []; 
const static unsigned int _bmp_nums_speed [];
const static unsigned int _bmp_mi []; 
const static unsigned int _bmp_km []; 
const static unsigned int _bmp_kmh [];
const static unsigned int _bmp_mph []; 

static unsigned int _dummy_mask[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
static MONO_SPR _font_spr_big   = { 24, 21, _bmp_nums_speed, _dummy_mask, 1,0};
static MONO_SPR _font_spr_small = { 12, 9, _bmp_nums_distance, _dummy_mask, 1,0};

static MONO_SPR _km_spr =  { 15, 7, _bmp_km, _dummy_mask, 1,0};
static MONO_SPR _mi_spr =  { 21, 10, _bmp_mi, _dummy_mask, 1,0};
static MONO_SPR _kmh_spr =  { 23, 7, _bmp_kmh, _dummy_mask, 1,0};
static MONO_SPR _mph_spr =  { 21, 10, _bmp_mph, _dummy_mask, 1,0};


static inline void _limit ( int* v, int min, int max)
{
    if ( *v < min) *v = 0;
    if ( *v > max) *v = max;
}

static inline int _xkutybit ( int x, int y, int t)
{
	int xx = x - 64; int yy = y - 32;
	int r = xx*xx + yy*yy;
	return ( r < t) ? 1 : 0;
}

void _xkuty_effect ( int t)
{
    int i=0, x=0, y=0, sx=0;
    for (y=0; y<64; y++)
	{
        for ( x=0; x<128; x+=32)
        {
			unsigned int pic = xkuty_bw [i];
        	int bits = _xkutybit ( x+0, y, t);
			for (sx=0; sx<32; sx++)
			{
				bits <<= 1;
				bits |= _xkutybit ( x+sx, y, t);
			}
			screen [i] = bits & pic;
            i++;
        }
	}
}

static inline int _sensor_scale ( int in, int base, int top)
{
	int len = top - base;
	int pos = in  - base;
	_limit ( &pos, 0, len);
	return (pos * 0x100) / len;
}


static void _clean_bilevel ( unsigned int* scr, int w, int h) 
{ 
	for (int i=0; i<((w*h)/32); i++) scr [i] = 0; 
}

void test_mono ();

static int frame = 0;
static char _tmp [20];

static void _draw_text ( const char* text, const MONO_SPR* font, int x, int y)
{ 
	MONO_SPR spr   = *font;
	int i = 0;
	while ( text[i])
	{
		int glyph_w = font->w;
		int glyph = -1;
		if (( text [i]>='0' && text [i]<='9'))
			glyph = text [i] - '0';
		else
			if ( text [i] == '.')	// Hyper-hack
				glyph = 10, glyph_w = font->w >> 1;
		if ( glyph != -1)
		{
			spr.bitmap = font->bitmap + (spr.stride_bitmap * spr.h * glyph);
			mono_draw_sprite ( screen, 128, 64, &spr, x, y);
		}
		x += glyph_w;
		i++;
	}
}


static void _vertical_sprite_comb ( const MONO_SPR* spr0, const MONO_SPR* spr1, 
                                    int level_fx8, int x, int y)
{
	MONO_SPR spr = *spr1;
	_limit ( &level_fx8, 0,0x100);
	int cut_y = (level_fx8 * spr.h) >> 8;
	spr.h     = cut_y;
	if ( spr.h > 0)
		mono_draw_sprite ( screen, 128, 64, &spr, x, y);

	spr = *spr0;
	spr.bitmap = spr0->bitmap + (spr.stride_bitmap * cut_y);
	spr.h      = spr.h - cut_y;
	if ( spr.h > 0)
		mono_draw_sprite ( screen, 128, 64, &spr, x, y + cut_y);
}

static void _battery_bar ( int level_fx8)
{
	int batt_x = 5, batt_y = 4;
	static MONO_SPR _battery_full  = { 27, 59, _bmp_battery, _dummy_mask, 1,0};
	static MONO_SPR _battery_empty = { 27, 59, _bmp_battery_empty, _dummy_mask, 1,0};

	_vertical_sprite_comb ( &_battery_full, &_battery_empty, level_fx8, 5, 4);
}



void main()
{
	int speed             = 0;
	int distance_hi       = 0;
	int distance_lo       = 0;
	int battery_level_fx8 = 0;

    int status = ST_LOGO_IN;
    int factor = 0;
    int i;

	lcd_initialize();
	lcdcon_gpo_backlight(1);

    hal_adc_initialize(1000, 16);

    unsigned long time = exos_timer_time();

    while (1)
    {
        volatile unsigned long time_c, time_b, time_a = exos_timer_time();

        _clean_bilevel ( screen, 128, 64);

        unsigned short ain[6];

        // 0 - Throtle  (16 bits)
        // 1 - Brake left (16 bits)
        // 2 - Brake right (16 bits)
        // 4 - Start (bool)
        // 5 - Horn  (bool)
        for(int i = 0; i < 6; i++)
            ain[i] = hal_adc_read(i);   

		// General switch; insert new screens here
        switch ( status)
        {
            case ST_LOGO_IN:
                _xkuty_effect ( factor * factor);
                factor++; if ( factor > 75) status = ST_LOGO_SHOW;
                break;

            case ST_LOGO_SHOW:
                _xkuty_effect ( 100*100);
                if ( frame == 2*80)
                    status = ST_LOGO_OUT;
                break;

            case ST_LOGO_OUT:
                _xkuty_effect ( factor * factor);
                factor--; 
                if ( factor == 0)
                   status = ST_DASH, frame = 0;
                break;

            case ST_DASH:

				speed       = frame & 0xff;
				distance_hi = frame;
				distance_lo = 7;
				int l = sprintf ( _tmp, "%d", speed);
				_draw_text ( _tmp, &_font_spr_big,   124 - (24*l), 13);
				l = sprintf ( _tmp, "%d", distance_hi);
				_tmp[l] = '.', l++;
				l += sprintf ( &_tmp[l], "%d", distance_lo);
				_draw_text ( _tmp, &_font_spr_small, 130 - (_font_spr_small.w * l), 50);
	
				battery_level_fx8 = frame & 0xff;
				_battery_bar ( battery_level_fx8);

                mono_draw_sprite ( screen, 128, 64, &_kmh_spr, 100,4);
                mono_draw_sprite ( screen, 128, 64, &_km_spr, 108,41);

                break;
         }

		//test_mono ();

		// Screen conversion
		_bilevel_linear_2_lcd ( scrrot, screen, 128, 64);

		time_b = exos_timer_time();

		lcd_dump_screen ((unsigned char*) scrrot);

		time_c = exos_timer_time();

		frame++;

		int elap = exos_timer_elapsed( time);
		time = exos_timer_time();
		int rem = 32 - elap;
		if ( rem > 0)
			exos_thread_sleep( rem);
    }
}

static inline void _bit_exchange ( unsigned * a, unsigned * b, unsigned mask, int shift)
{
    unsigned r;
    r  = (*a & mask) | ((*b & mask) << shift);
    *b = ((*a >> shift) & mask) | (*b & (~mask));
    *a = r;
}

static inline void _tile_rotate ( unsigned char* dst, unsigned int* src, int src_stride)
{
    unsigned d0, d1, d2, d3, d4, d5, d6, d7;
    d0 = *src; src += src_stride;
    d1 = *src; src += src_stride;
    d2 = *src; src += src_stride;
    d3 = *src; src += src_stride;
    d4 = *src; src += src_stride;
    d5 = *src; src += src_stride;
    d6 = *src; src += src_stride;
    d7 = *src; src += src_stride;

    _bit_exchange ( &d0, &d4, 0x0f0f0f0f, 4);
    _bit_exchange ( &d1, &d5, 0x0f0f0f0f, 4);
    _bit_exchange ( &d2, &d6, 0x0f0f0f0f, 4);
    _bit_exchange ( &d3, &d7, 0x0f0f0f0f, 4);

    _bit_exchange ( &d0, &d2, 0x33333333, 2);
    _bit_exchange ( &d1, &d3, 0x33333333, 2);
    _bit_exchange ( &d4, &d6, 0x33333333, 2);
    _bit_exchange ( &d5, &d7, 0x33333333, 2);

    _bit_exchange ( &d0, &d1, 0x55555555, 1);
    _bit_exchange ( &d2, &d3, 0x55555555, 1);
    _bit_exchange ( &d4, &d5, 0x55555555, 1);
    _bit_exchange ( &d6, &d7, 0x55555555, 1);

    dst [0]  = d7 >> 24;  dst [1] = d6 >> 24;   dst [2] = d5 >> 24;   dst [3] = d4 >> 24;
    dst [4]  = d3 >> 24;  dst [5] = d2 >> 24;   dst [6] = d1 >> 24;   dst [7] = d0 >> 24;
    dst [8]  = d7 >> 16;  dst [9]  = d6 >> 16;  dst [10] = d5 >> 16;  dst [11] = d4 >> 16;
    dst [12] = d3 >> 16;  dst [13] = d2 >> 16;  dst [14] = d1 >> 16;  dst [15] = d0 >> 16;
    dst [16] = d7 >> 8;   dst [17] = d6 >> 8;   dst [18] = d5 >> 8;   dst [19] = d4 >> 8;
    dst [20] = d3 >> 8;   dst [21] = d2 >> 8;   dst [22] = d1 >> 8;   dst [23] = d0 >> 8;
    dst [24] = d7 >> 0;   dst [25] = d6 >> 0;   dst [26] = d5 >> 0;   dst [27] = d4 >> 0;
    dst [28] = d3 >> 0;   dst [29] = d2 >> 0;   dst [30] = d1 >> 0;   dst [31] = d0 >> 0;
}

static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h)
{
    int bar, x;
    const int bars = h >> 3;
    for (bar=0; bar<bars; bar++)
    {
        unsigned char* dst_bar_ptr = &dst [ bar * w];
        unsigned int*  src_bar_ptr = &src [ bar * 8 * (w>>5)];
        for (x=0; x<w; x+=32)
            _tile_rotate ( &dst_bar_ptr[x], 
                           &src_bar_ptr[x>>5], w >> 5);
    }
}

// -----------------------------------------------------------------------

const static unsigned int _bmp_mi [] = 
{
0x0, 0x0, 0x1000, 0x0, 
0x361000, 0x491000, 0x491000, 0x491000, 
0x0, 0x0
};
const static unsigned int _bmp_km [] = 
{
0x0, 0x48000000, 0x50d80000, 0x61240000, 
0x51240000, 0x49240000, 0x0
};
const static unsigned int _bmp_kmh [] = 
{
0x4000, 0x24004000, 0x286c7000, 0x30924800, 
0x28924800, 0x24924800, 0x0
};
const static unsigned int _bmp_mph [] = 
{
0x0, 0x8000, 0x8000, 0x3638e000, 
0x49249000, 0x49249000, 0x49389000, 0x200000, 
0x200000, 0x0
};

const static unsigned int _bmp_battery [] = 
{
0x0, 0x0, 0x1fff000, 0x1fff000, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x0, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x0, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x0, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x0, 0x0
};
const static unsigned int _bmp_battery_empty [] = 
{ 
0x0, 0x0, 0x1fff000, 0x1803000, 
0x3f803f80, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x3fffff80, 
0x0, 0x3fffff80, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x3fffff80, 0x0, 0x3fffff80, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x3fffff80, 0x0, 
0x3fffff80, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x3fffff80, 
0x3fffff80, 0x0, 0x0
};

const static unsigned int _bmp_nums_speed [] = 
{ 
0xfffc000, 0x3ffff000, 0x7ffff800, 0x7ffff800, 
0xf8007c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf0003c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf0003c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf8007c00, 0x7ffff800, 0x7ffff800, 0x3ffff000, 
0xfffc000, 0x7fc0000, 0x7fc0000, 0x7fc0000, 
0x7fc0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x7ffe000, 0x7ffe000, 
0x7ffe000, 0x7ffe000, 0x3fffc000, 0x3ffff000, 
0x3ffff800, 0x3ffff800, 0x7c00, 0x3c00, 
0x3c00, 0x7c00, 0xffff800, 0x3ffff800, 
0x7ffff000, 0x7fffc000, 0xf8000000, 0xf0000000, 
0xf0000000, 0xf0000000, 0xf0000000, 0xfffff800, 
0xfffff800, 0xfffff800, 0xfffff800, 0x3fff0000, 
0x3fffc000, 0x3fffe000, 0x3fffe000, 0x1f000, 
0xf000, 0xf000, 0x1f000, 0x7ff000, 
0x7fe000, 0x7fe000, 0x7ff000, 0xf800, 
0x7800, 0x7800, 0x7800, 0xf800, 
0x3ffff000, 0x3ffff000, 0x3fffe000, 0x3fff8000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf801e000, 0xfffffc00, 0x7ffffc00, 0x3ffffc00, 
0x1ffffc00, 0x1e000, 0x1e000, 0x1e000, 
0x1e000, 0xfffff800, 0xfffff800, 0xfffff800, 
0xfffff800, 0xf0000000, 0xf0000000, 0xf0000000, 
0xf8000000, 0x7fff8000, 0x7fffe000, 0x3ffff000, 
0xffff000, 0xf800, 0x7800, 0x7800, 
0x7800, 0xf800, 0x3ffff000, 0x3ffff000, 
0x3fffe000, 0x3fff8000, 0x7fff800, 0x1ffff800, 
0x3ffff800, 0x3ffff800, 0x7c000000, 0x78000000, 
0x78000000, 0x7fffc000, 0x7ffff000, 0x7ffff800, 
0x7ffff800, 0x78007c00, 0x78003c00, 0x78003c00, 
0x78003c00, 0x78003c00, 0x7c007c00, 0x3ffff800, 
0x3ffff800, 0x1ffff000, 0x7ffc000, 0xfffff800, 
0xfffff800, 0xfffff800, 0xfffff800, 0xf000f000, 
0xf001e000, 0x3e000, 0x7c000, 0xf8000, 
0x1f0000, 0x1e0000, 0x3c0000, 0x7c0000, 
0xf80000, 0x1f00000, 0x1e00000, 0x1e00000, 
0x1e00000, 0x1e00000, 0x1e00000, 0x1e00000, 
0x7ffc000, 0x1ffff000, 0x3ffff800, 0x3ffff800, 
0x7c007c00, 0x78003c00, 0x78003c00, 0x7c007c00, 
0x3ffff800, 0x3ffff800, 0x3ffff800, 0x7ffffc00, 
0xfc003c00, 0xf8001c00, 0xf8001c00, 0xf8001c00, 
0xfc003c00, 0x7ffffc00, 0x7ffffc00, 0x3ffff800, 
0xfffe000, 0xfff8000, 0x3fffe000, 0x7ffff000, 
0x7ffff000, 0xf800f800, 0xf0007800, 0xf0007800, 
0xf0007800, 0xf0007800, 0xf8007800, 0x7ffff800, 
0x7ffff800, 0x3ffff800, 0xffff800, 0x7800, 
0x7800, 0xf800, 0x3ffff000, 0x3ffff000, 
0x3fffe000, 0x3fff8000
};

const static unsigned int _bmp_nums_distance [] = 
{ 
0x7f800000, 0xffc00000, 0xc0c00000, 0xc0c00000, 
0xc0c00000, 0xc0c00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0x3c000000, 0x3c000000, 0xc000000, 
0xc000000, 0xc000000, 0xc000000, 0xc000000, 
0x3f000000, 0x3f000000, 0x7f800000, 0x7fc00000, 
0xc00000, 0x7fc00000, 0xff800000, 0xc0000000, 
0xc0000000, 0xffc00000, 0xffc00000, 0x3f800000, 
0x3fc00000, 0xc00000, 0x7c00000, 0x7c00000, 
0xc00000, 0xc00000, 0x3fc00000, 0x3f800000, 
0xc1800000, 0xc1800000, 0xc1800000, 0xc1800000, 
0xc1800000, 0xffc00000, 0x7fc00000, 0x1800000, 
0x1800000, 0xffc00000, 0xffc00000, 0xc0000000, 
0xff800000, 0x7fc00000, 0xc00000, 0xc00000, 
0xffc00000, 0xff800000, 0x7f800000, 0xff800000, 
0xc0000000, 0xff800000, 0xffc00000, 0xc0c00000, 
0xc0c00000, 0xffc00000, 0x7f800000, 0xffc00000, 
0xffc00000, 0x81c00000, 0x3800000, 0x7000000, 
0xe000000, 0x1c000000, 0x18000000, 0x18000000, 
0x7f800000, 0xffc00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0xc0c00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0x7f800000, 0xffc00000, 0xc0c00000, 
0xc0c00000, 0xffc00000, 0x7fc00000, 0xc00000, 
0x7fc00000, 0x7f800000, 0x0, 0x0, 
0x0, 0x0, 0x0,
0x0, 0x0, 0x18000000, 0x18000000
};



static const unsigned int xkuty_bw [] = 
{ 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x400, 0x0, 0x0, 0x0, 
0xffe0, 0x0, 0x0, 0x7c000000, 
0xfff8, 0x0, 0x0, 0x7c000000, 
0x8fffe, 0x0, 0x0, 0x7c000000, 
0x1c7fff, 0x0, 0x0, 0x7c000000, 
0x3c7fff, 0x803c078f, 0x3c780f1, 0xff9e01f0, 
0x7e007f, 0xc03e0f9f, 0x7c7c1f3, 0xffdf01f0, 
0xfe001f, 0xc01e1f1f, 0xf87c1f3, 0xffdf03e0, 
0xfc0007, 0xe01f1f1f, 0x1f07c1f3, 0xffcf83e0, 
0x1f80003, 0xf00fbe1f, 0x1e07c1f0, 0x7c0f83e0, 
0x1f80703, 0xf00fbc1f, 0x3e07c1f0, 0x7c07c7c0, 
0x3f00781, 0xf007fc1f, 0x7c07c1f0, 0x7c07c7c0, 
0x3f043c1, 0xf803f81f, 0xf807c1f0, 0x7c07c780, 
0x3e0c1e0, 0xf803f81f, 0xfc07c1f0, 0x7c03ef80, 
0x3e0c0e0, 0xf803f81f, 0xfe07c1f0, 0x7c03ef80, 
0x3e0c0e0, 0xf807fc1f, 0xfe07c1f0, 0x7c01ff00, 
0x3e0c060, 0xf807fc1f, 0x9f07c1f0, 0x7c01ff00, 
0x3e0e0e0, 0xf80fbe1f, 0xf87c1f0, 0x7c01ff00, 
0x3e0f1e0, 0xf80f1e1f, 0xf87e7f0, 0x7e00fe00, 
0x3f07fc1, 0xf81f1f1f, 0x7c3fff0, 0x7fc0fe00, 
0x3f03f81, 0xf03e0f9f, 0x3e3fef0, 0x3fc07e00, 
0x1f80c03, 0xf03e0f9f, 0x3e1fcf0, 0x1fc07c00, 
0x1f80007, 0xf0000000, 0x6000, 0x7807c00, 
0xfc000f, 0xe0000000, 0x0, 0x7c00, 
0xff001f, 0xc0000000, 0x0, 0xf800, 
0x7fc07f, 0xc0000000, 0x0, 0xf800, 
0x3fffff, 0x80000000, 0x0, 0x1f000, 
0x1fffff, 0x0, 0x0, 0xff000, 
0xffffc, 0x0, 0x0, 0x1fe000, 
0x3fff8, 0x0, 0x0, 0x1fc000, 
0xffe0, 0x0, 0x0, 0x1f8000, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0
};

const short tabsin [512] = {
0, 201, 402, 603, 803, 1004, 1205, 1405, 
1605, 1805, 2005, 2204, 2404, 2602, 2801, 2998, 
3196, 3393, 3589, 3785, 3980, 4175, 4369, 4563, 
4756, 4948, 5139, 5329, 5519, 5708, 5896, 6083, 
6269, 6455, 6639, 6822, 7005, 7186, 7366, 7545, 
7723, 7900, 8075, 8249, 8423, 8594, 8765, 8934, 
9102, 9268, 9434, 9597, 9759, 9920, 10079, 10237, 
10393, 10548, 10701, 10853, 11002, 11150, 11297, 11442, 
11585, 11726, 11866, 12003, 12139, 12273, 12406, 12536, 
12665, 12791, 12916, 13038, 13159, 13278, 13395, 13510, 
13622, 13733, 13842, 13948, 14053, 14155, 14255, 14353, 
14449, 14543, 14634, 14723, 14810, 14895, 14978, 15058, 
15136, 15212, 15286, 15357, 15426, 15492, 15557, 15618, 
15678, 15735, 15790, 15842, 15892, 15940, 15985, 16028, 
16069, 16107, 16142, 16175, 16206, 16234, 16260, 16284, 
16305, 16323, 16339, 16353, 16364, 16372, 16379, 16382, 
16383, 16382, 16379, 16372, 16364, 16353, 16339, 16323, 
16305, 16284, 16260, 16234, 16206, 16175, 16142, 16107, 
16069, 16028, 15985, 15940, 15892, 15842, 15790, 15735, 
15678, 15618, 15557, 15492, 15426, 15357, 15286, 15212, 
15136, 15058, 14978, 14895, 14810, 14723, 14634, 14543, 
14449, 14353, 14255, 14155, 14053, 13948, 13842, 13733, 
13622, 13510, 13395, 13278, 13159, 13038, 12916, 12791, 
12665, 12536, 12406, 12273, 12139, 12003, 11866, 11726, 
11585, 11442, 11297, 11150, 11002, 10853, 10701, 10548, 
10393, 10237, 10079, 9920, 9759, 9597, 9434, 9268, 
9102, 8934, 8765, 8594, 8423, 8249, 8075, 7900, 
7723, 7545, 7366, 7186, 7005, 6822, 6639, 6455, 
6269, 6083, 5896, 5708, 5519, 5329, 5139, 4948, 
4756, 4563, 4369, 4175, 3980, 3785, 3589, 3393, 
3196, 2998, 2801, 2602, 2404, 2204, 2005, 1805, 
1605, 1405, 1205, 1004, 803, 603, 402, 201, 
0, -201, -402, -603, -803, -1004, -1205, -1405, 
-1605, -1805, -2005, -2204, -2404, -2602, -2801, -2998, 
-3196, -3393, -3589, -3785, -3980, -4175, -4369, -4563, 
-4756, -4948, -5139, -5329, -5519, -5708, -5896, -6083, 
-6269, -6455, -6639, -6822, -7005, -7186, -7366, -7545, 
-7723, -7900, -8075, -8249, -8423, -8594, -8765, -8934, 
-9102, -9268, -9434, -9597, -9759, -9920, -10079, -10237, 
-10393, -10548, -10701, -10853, -11002, -11150, -11297, -11442, 
-11585, -11726, -11866, -12003, -12139, -12273, -12406, -12536, 
-12665, -12791, -12916, -13038, -13159, -13278, -13395, -13510, 
-13622, -13733, -13842, -13948, -14053, -14155, -14255, -14353, 
-14449, -14543, -14634, -14723, -14810, -14895, -14978, -15058, 
-15136, -15212, -15286, -15357, -15426, -15492, -15557, -15618, 
-15678, -15735, -15790, -15842, -15892, -15940, -15985, -16028, 
-16069, -16107, -16142, -16175, -16206, -16234, -16260, -16284, 
-16305, -16323, -16339, -16353, -16364, -16372, -16379, -16382, 
-16383, -16382, -16379, -16372, -16364, -16353, -16339, -16323, 
-16305, -16284, -16260, -16234, -16206, -16175, -16142, -16107, 
-16069, -16028, -15985, -15940, -15892, -15842, -15790, -15735, 
-15678, -15618, -15557, -15492, -15426, -15357, -15286, -15212, 
-15136, -15058, -14978, -14895, -14810, -14723, -14634, -14543, 
-14449, -14353, -14255, -14155, -14053, -13948, -13842, -13733, 
-13622, -13510, -13395, -13278, -13159, -13038, -12916, -12791, 
-12665, -12536, -12406, -12273, -12139, -12003, -11866, -11726, 
-11585, -11442, -11297, -11150, -11002, -10853, -10701, -10548, 
-10393, -10237, -10079, -9920, -9759, -9597, -9434, -9268, 
-9102, -8934, -8765, -8594, -8423, -8249, -8075, -7900, 
-7723, -7545, -7366, -7186, -7005, -6822, -6639, -6455, 
-6269, -6083, -5896, -5708, -5519, -5329, -5139, -4948, 
-4756, -4563, -4369, -4175, -3980, -3785, -3589, -3393, 
-3196, -2998, -2801, -2602, -2404, -2204, -2005, -1805, 
-1605, -1405, -1205, -1004, -803, -603, -402, -201};

#if 0

const static unsigned int new_bmp_switch_on [] = 
{ 
0x0,
0x0,
0x0,
0x007f0000, //0x00, 0x7f, 0x00, 
0x01c1c000, //0x01, 0xc1, 0xc0, 
0x03146000, //0x03, 0x14, 0x60, 
0x06aab000, //0x06, 0xaa, 0xb0, 
0x0d555800, //0x0d, 0x55, 0x58, 
0x0affa800, //0x0a, 0xff, 0xa8, 
0x1bffec00, //0x1b, 0xff, 0xec, 
0x17fff400, //0x17, 0xff, 0xf4, 
0x1ffffc00, //0x1f, 0xff, 0xfc, 
0x1ffffc00, //0x1f, 0xff, 0xfc, 
0x1ffffc00, //0x1f, 0xff, 0xfc, 
0x1ffffc00, //0x1f, 0xff, 0xfc, 
0x0ffff800, //0x0f, 0xff, 0xf8, 
0x0ffff800, //0x0f, 0xff, 0xf8, 
0x07fff000, //0x07, 0xff, 0xf0, 
0x03ffe000, //0x03, 0xff, 0xe0, 
0x01ffc000, //0x01, 0xff, 0xc0 
0x007f0000, //0x00, 0x7f, 0x00, 
0x0, 
0x0, 
0x0 
};

const static unsigned int new_bmp_switch_on_mask [] = 
{ 
0x0,0x0,
0x0,
0x007f8000,  
0x01ffe000,
0x03ffe000, 
0x07fff000,
0x0ffff800,
0x0ffff800, 
0x1ffffc00,
0x1ffffc00,
0x1ffffc00, 
0x1ffffc00, 
0x1ffffc00,
0x1ffffc00,
0x0ffff800, 
0x0ffff800, 
0x07fff000,
0x03ffe000, 
0x01ffc000, 
0x007f0000,
0x0, 
0x0, 0x0 
};


unsigned int kkmask[] = {0xffffffff, 0xffffffff, 0xffffffff};
void test_mono ()
{
	int i;
	static int ang=0;
	//MONO_SPR spr = { 24, 24, new_bmp_switch_on, &kkmask, 1,0};
	//MONO_SPR spr = { 24, 24, new_bmp_switch_on, new_bmp_switch_on_mask, 1,1};
	MONO_SPR spr = { 22, 21, _bmp_nums_speed, kkmask, 1,0};

	for (i=0; i<1; i++)
	{
		int x = -12 + 64 + ((tabsin[((ang+(i*12))+0)   & 0x1ff] * 70) >> 14);
		int y = -12 + 32 + ((tabsin[((ang+(i*12))+128) & 0x1ff] * 30) >> 14);
		mono_draw_sprite ( screen, 128, 64, &spr, x, y);
	}
	ang += 3;
}

#endif

#if 0
#define PHI  ((1.0f+2.23f)/2.0f)
#define BB  (short)((1.0f/PHI)*30.0f) 
#define CC  (short)((2.0f-PHI)*30.0f)
#define ONE (short)(30.0f)
static const short dode [ 12 * 5 * 3] =
{
 CC,  0,  ONE,   BB,  BB,  BB,  0,  ONE,  CC, -BB,  BB,  BB, -CC,  0,  ONE,
-CC,  0,  ONE,  -BB, -BB,  BB,  0, -ONE,  CC,  BB, -BB,  BB,  CC,  0,  ONE,
 CC,  0, -ONE,   BB, -BB, -BB,  0, -ONE, -CC, -BB, -BB, -BB, -CC,  0, -ONE,
-CC,  0, -ONE,  -BB,  BB, -BB,  0,  ONE, -CC,  BB,  BB, -BB,  CC,  0, -ONE,
 0,  ONE, -CC,   0,  ONE,  CC,  BB,  BB,  BB,  ONE,  CC,  0,  BB,  BB, -BB,
 0,  ONE,  CC,   0,  ONE, -CC, -BB,  BB, -BB, -ONE,  CC,  0, -BB,  BB,  BB,
 0, -ONE, -CC,   0, -ONE,  CC, -BB, -BB,  BB, -ONE, -CC,  0, -BB, -BB, -BB,
 0, -ONE,  CC,   0, -ONE, -CC,  BB, -BB, -BB,  ONE, -CC,  0,  BB, -BB,  BB,
 ONE,  CC,  0,   BB,  BB,  BB,  CC,  0,  ONE,  BB, -BB,  BB,  ONE, -CC,  0, 
 ONE, -CC,  0,   BB, -BB, -BB,  CC,  0, -ONE,  BB,  BB, -BB,  ONE,  CC,  0,
-ONE,  CC,  0,  -BB,  BB, -BB,  -CC,  0, -ONE, -BB, -BB, -BB, -ONE, -CC, 0,
-ONE, -CC,  0,  -BB, -BB,  BB, -CC,  0,  ONE, -BB,  BB,  BB, -ONE,  CC,  0
};

static const unsigned int pattern1 [8] = { 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,0x55555555};
static const unsigned int pattern0 [8] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,0xffffffff};

static inline void spin2d ( int* x, int *y, int sn, int cs)
{
	int xx = (*x * cs - *y * sn) >> 12;
	    *y = (*x * sn + *y * cs) >> 12;
		*x = xx;
}

static inline void spin3d ( int* x, int* y, int* z, int* trig)
{
	spin2d ( y, z, trig[0], trig[1]);
	spin2d ( x, z, trig[2], trig[3]);
	spin2d ( x, y, trig[4], trig[5]);
}

static int axis_x = 0, axis_y = 0, axis_z = 0;
static int zoom = 0;

static MONO_POLY_COORDS penta[] = {{0, 30}, {28, 9}, {17, -24}, {-17, -24}, {-28, 9}};

static int poly_init = 0;
static MONO_POLY polygon;
static MONO_POLY_COORDS coords[] = {{64,2},{104,32},{84,52},{44,52},{24,32}};

void test_mono ()
{
	int trig [6] = { tabsin[axis_x & 511] >> 2, tabsin[ (axis_x+128) & 511] >> 2,
	                 tabsin[axis_y & 511] >> 2, tabsin[ (axis_y+128) & 511] >> 2,
					 tabsin[axis_z & 511] >> 2, tabsin[ (axis_z+128) & 511] >> 2};

	if ( poly_init == 0)
		mono_filled_polygon_create ( &polygon, 128, 64), poly_init = 1;

	int sc = 0x1800 + ( tabsin[zoom & 511]>>4);
	for ( int j=0; j<12; j++) 
		for ( int i=0; i<5; i++)
		{
			int x = dode[j*5*3+i*3+0];//penta[i].x;
			int y = dode[j*5*3+i*3+1];//penta[i].y;
			int z = dode[j*5*3+i*3+2];//0;
			spin3d(&x,&y,&z, trig);
			coords[i].x = 64 + ((x * sc) >> 12);
			coords[i].y = 32 + ((y * sc) >> 12);

			mono_filled_polygon ( &polygon, screen, (j&1) ? pattern0 : pattern0, 
								  coords, sizeof(coords)/sizeof(MONO_POLY_COORDS));
		}

	axis_x += 1;
	axis_y += 2;
	axis_z += 3;
	zoom   += 10;
}
#endif
