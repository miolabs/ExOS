#include <support/lcd/lcd.h>
#include <support/lcd/mono.h>
#include <support/adc_hal.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <stdio.h>
#include <assert.h>


static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static const CAN_EP _eps[] = {
	{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };

static EXOS_EVENT _can_event;


static unsigned char scrrot [(128*64)/8];
static unsigned int  screen [(128*64)/32];

static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h);

static void _clean_bilevel ( unsigned int* scr, int w, int h) 
{ 
	for (int i=0; i<((w*h)/32); i++) scr [i] = 0; 
}

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
	int frame = 0;
	int speed             = 0;
	int distance_hi       = 0;
	int distance_lo       = 0;
	int battery_level_fx8 = 0;

    int status = ST_LOGO_IN;
    int factor = 0;
    int i;

	exos_event_create(&_can_event);
	hal_can_initialize(LCD_CAN_BUS, 250000);
	hal_fullcan_setup(_can_setup, NULL);

	lcd_initialize();
	lcdcon_gpo_backlight(1);

	hal_adc_initialize(1000, 16);

    unsigned int time_base = exos_timer_time();
	while(1)
	{
		if (0 == exos_event_wait(&_can_event, 100))
		{
		}
		else
		{
			unsigned int time = exos_timer_time();
			time -= time_base;

			unsigned short ain[6];
			// 0 - Throtle  (16 bits)
			// 1 - Brake left (16 bits)
			// 2 - Brake right (16 bits)
			// 4 - Start (bool)
			// 5 - Horn  (bool)
			for(int i = 0; i < 6; i++)
				ain[i] = hal_adc_read(i);

			unsigned char relays = 0;
			if (ain[3] < 0x8000) relays |= (1<<0);
			if (ain[4] < 0x8000) relays |= (1<<1);

			CAN_BUFFER buf = (CAN_BUFFER) { relays, 2, 3, 4, 5, 6, 7, 8 };
			hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);

			// Show inputs
			_clean_bilevel ( screen, 128, 64);

			// General switch; insert new screens here
			switch ( status)
			{
				case ST_LOGO_IN:
					factor = time/16;
					_xkuty_effect ( factor * factor);
					if ( factor > 75) status = ST_LOGO_SHOW;
					break;

				case ST_LOGO_SHOW:
					_xkuty_effect ( 100*100);
					if ( time >= 1000*2)
						status = ST_LOGO_OUT;
					break;

				case ST_LOGO_OUT:
                	factor = 75-((time-1000*2)/16);
					_xkuty_effect ( factor * factor);
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

			// Screen conversion & dump
			_bilevel_linear_2_lcd ( scrrot, screen, 128, 64);
			lcd_dump_screen ( scrrot);

			frame++;
		}	// if (0 == exos_event_wait(&_can_event, 100))
	}	// while (1)
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	// TODO: process msg
	switch(msg->EP.Id)
	{
		case 0x300:
//			RELAY_PORT->MASKED_ACCESS[RELAY1] = (msg->Data.u8[0] & (1<<0)) ? RELAY1 : 0;
//			RELAY_PORT->MASKED_ACCESS[RELAY2] = (msg->Data.u8[0] & (1<<1)) ? RELAY2 : 0;
			break;
	}

	exos_event_reset(&_can_event);
}

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state)
{
	int count = sizeof(_eps) / sizeof(CAN_EP);
	if (index < count)
	{
		*pflags = CANF_RXINT;
		*ep = _eps[index];
		return 1;
	}
	return 0;
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


// Bitmaps

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


