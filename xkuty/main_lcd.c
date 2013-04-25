#include <support/lcd/lcd.h>
#include <support/lcd/mono.h>
#include <support/adc_hal.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <stdio.h>
#include <assert.h>
#include "xcpu.h"
#include "fir.h"


static unsigned char scrrot [(128*64)/8];
static unsigned int  screen [(128*64)/32];

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h);
static void _clean_bilevel ( unsigned int* scr, int w, int h)  { for (int i=0; i<((w*h)/32); i++) scr [i] = 0;  }

static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };


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
	//
    ST_DASH,
	ST_DEBUG,
};

static const unsigned int xkuty_bw [];
static const unsigned int xkuty2_bw [];
static const unsigned int exos_bw [];
const static unsigned int _bmp_battery [];
const static unsigned int _bmp_battery_empty [];
const static unsigned int _bmp_nums_distance []; 
const static unsigned int _bmp_nums_speed [];
const static unsigned int _bmp_mi []; 
const static unsigned int _bmp_km []; 
const static unsigned int _bmp_kmh [];
const static unsigned int _bmp_mph []; 
const static unsigned int _bmp_lock [];
const static unsigned int _bmp_warning [];
const static unsigned int _bmp_fatal_error [];
const static unsigned int _bmp_cruisin [];

static unsigned int _dummy_mask[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

static MONO_SPR _font_spr_big   = { 24, 21, _bmp_nums_speed, _dummy_mask, 1,0};
static MONO_SPR _font_spr_small = { 12, 9, _bmp_nums_distance, _dummy_mask, 1,0};

static MONO_SPR _km_spr =  { 15, 7, _bmp_km, _dummy_mask, 1,0};
static MONO_SPR _mi_spr =  { 21, 10, _bmp_mi, _dummy_mask, 1,0};
static MONO_SPR _kmh_spr =  { 23, 7, _bmp_kmh, _dummy_mask, 1,0};
static MONO_SPR _mph_spr =  { 21, 10, _bmp_mph, _dummy_mask, 1,0};
static MONO_SPR _lock_spr =  { 30,25, _bmp_lock, _dummy_mask, 1,0};
static MONO_SPR _warning_spr =  { 24,21, _bmp_warning, _dummy_mask, 1,0};
static MONO_SPR _fatal_error_spr =  { 32,24, _bmp_fatal_error, _dummy_mask, 1,0};
static MONO_SPR _cruisin_spr =  { 30,26, _bmp_cruisin, _dummy_mask, 1,0};


static inline void _limit ( int* v, int min, int max)
{
    if ( *v < min) *v = 0;
    if ( *v > max) *v = max;
}

static inline int _xkutybit ( int x, int y, int t, int cx, int cy)
{
	int xx = x - cx; int yy = y - cy;
	int r = xx*xx + yy*yy;
	return ( r < t) ? 1 : 0;
}

void _xkuty_effect ( const unsigned int* bitmap, int t, int cx, int cy)
{
    int i=0, x=0, y=0, sx=0;
    for (y=0; y<64; y++)
	{
        for ( x=0; x<128; x+=32)
        {
			unsigned int pic = bitmap [i];
        	int bits = _xkutybit ( x+0, y, t, cx, cy);
			for (sx=0; sx<32; sx++)
			{
				bits <<= 1;
				bits |= _xkutybit ( x+sx, y, t, cx, cy);
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
	int res = (pos * 0x100) / len;
	_limit ( &res, 0, 0xff);
	return res;
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

	_vertical_sprite_comb ( &_battery_full, &_battery_empty, 0x100 - level_fx8, 5, 4);
}


typedef struct
{
	unsigned short curr;				// Analogic inputs, 12 bits
    unsigned short scaled;				// Input re-escaled to def_min/def_max
	unsigned short max, min;			// Run time register
	unsigned short def_min, def_max;	// Factory registered min and max
	unsigned short filtered;			// Filtered value of curr
	FIR			   fir;					
} ANALOGIC;

#define NUM_ADC_INPUTS (5)
static ANALOGIC _ain[NUM_ADC_INPUTS]=
{
	{0,0,0,0xffff, 587, 3109},	// 0 - Throtle  (16 bits)
	{0,0,0,0xffff, 1765,2400},	// 1 - Brake left (16 bits)
	{0,0,0,0xffff, 1765,2700},	// 2 - Brake right (16 bits)
	{0,0,0,0xffff, 0,4095},		// 3 - Start (bool)
	{0,0,0,0xffff, 0,4095},		// 4 - Horn  (bool)
};

static int _fir_needs_init = 1;

#define INTRO_BMP1 xkuty2_bw
//#define INTRO_BMP2 xkuty2_bw
#define INTRO_BMP2 exos_bw

static EXOS_TIMER _timer_update;	// Could be local?

static void _intro ( int* status, int* st_time_base, int time)
{
	int factor;
	switch ( *status)
	{
		case ST_LOGO_IN:
			factor = time/6;
			_xkuty_effect ( INTRO_BMP1, factor * factor, 64, 32);
			if ( factor > 75) *status = ST_LOGO_SHOW, *st_time_base = time;
			break;
		case ST_LOGO_SHOW:
			_xkuty_effect ( INTRO_BMP1, 100*100, 64, 32);
			if ( time >= (*st_time_base+600)) *status = ST_LOGO_OUT, *st_time_base = time;
			break;
		case ST_LOGO_OUT:
			factor = 75-((time - *st_time_base) / 6);
			_xkuty_effect ( INTRO_BMP1, factor * factor, 64, 32);
			if ( factor <= 0) *status = ST_EXOS_IN, *st_time_base = time;
			break;
		case ST_EXOS_IN:
			factor = ((time - *st_time_base)/3);
			_xkuty_effect ( INTRO_BMP2, factor * factor, 0, 64);
			if ( factor > 140) *status = ST_EXOS_SHOW, *st_time_base = time;
			break;
		case ST_EXOS_SHOW:
			_xkuty_effect ( INTRO_BMP2, 150*150, 0, 64);
			if ( time >= (*st_time_base+600)) *status = ST_EXOS_OUT, *st_time_base = time;
			break;
		case ST_EXOS_OUT:
			factor = 140-((time - *st_time_base)/3);
			_xkuty_effect ( INTRO_BMP2, factor * factor, 127, 64);
			if ( factor <= 0) *status = ST_DASH;
			break;
	}
}


typedef struct {
	int status;
	int speed;
	int distance_hi;
	int distance_lo;
	int battery_level_fx8;
} DASH_DATA;

static DASH_DATA _dash = {0,0,0,0,0};

static void _runtime_screens ( int status)
{
	// General runtime switch; insert new screens here
	switch ( status)
	{
		case ST_DASH:
			{
				//_dash.status |= XCPU_STATE_CRUISE_ON;
				//_dash.status |= XCPU_STATE_WARNING;
				int l;
				l = sprintf ( _tmp, "%d", _dash.speed);
				if ( _dash.status & (XCPU_STATE_NEUTRAL | XCPU_STATE_ERROR))
				{
					if ( _dash.status & XCPU_STATE_NEUTRAL)
						mono_draw_sprite ( screen, 128, 64, &_lock_spr, 100,7);
					if ( _dash.status & XCPU_STATE_ERROR)
						mono_draw_sprite ( screen, 128, 64, &_fatal_error_spr, 60,9);
				}
				else
					_draw_text ( _tmp, &_font_spr_big,   124 - (24*l), 9);

				l = sprintf ( _tmp, "%d", _dash.distance_hi);
				_tmp[l] = '.', l++;
				l += sprintf ( &_tmp[l], "%d", _dash.distance_lo);
				_draw_text ( _tmp, &_font_spr_small, 130 - (_font_spr_small.w * l), 50);

				_battery_bar ( _dash.battery_level_fx8);

				if ( _dash.status & XCPU_STATE_CRUISE_ON)
					mono_draw_sprite ( screen, 128, 64, &_cruisin_spr, 36,4);
				if ( _dash.status & XCPU_STATE_WARNING)
					mono_draw_sprite ( screen, 128, 64, &_warning_spr, 36,26);

				//mono_draw_sprite ( screen, 128, 64, &_kmh_spr, 100,4);
				mono_draw_sprite ( screen, 128, 64, &_km_spr, 108,41);
			}
			break;

		case ST_DEBUG:
			for (int i=0; i<NUM_ADC_INPUTS; i++)
			{
				int y=12*i;
				/*if(( frame>>4) & 1)
					sprintf ( _tmp, "%d.%d %d", i, _ain[i].curr,_ain[i].max);
				else
					sprintf ( _tmp, "%d.%d %d", i, _ain[i].curr,_ain[i].min);*/
				sprintf ( _tmp, "%d.%d %d", i, _ain[i].filtered, _ain[i].scaled);
				_draw_text ( _tmp, &_font_spr_small, 0, y);
			}
			break;
	}
}


static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];

static void _get_can_messages ()
{
	XCPU_MSG *xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	while (xmsg != NULL)
	{
		switch(xmsg->CanMsg.EP.Id)
		{
			case 0x300:
				_dash.speed             = xmsg->CanMsg.Data.u8[0];
				_dash.battery_level_fx8 = xmsg->CanMsg.Data.u8[1];
				_dash.status       = xmsg->CanMsg.Data.u8[2];
				_dash.distance_hi       = xmsg->CanMsg.Data.u32[1] / 10;
				_dash.distance_lo       = xmsg->CanMsg.Data.u32[1] % 10;
				break;
		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	}
}

static void _read_send_analogic_inputs ()
{
	if ( _fir_needs_init)
	{
		_fir_needs_init = 0;
		for(int i=0; i<NUM_ADC_INPUTS; i++)
			fir_init( &_ain[0].fir, 500, 3);
	}

	static char discards [NUM_ADC_INPUTS] = {1,1,1,0,0};
	for(int i=0; i<NUM_ADC_INPUTS; i++)
	{
		_ain[i].curr   = hal_adc_read(i) >> 4;	// 12 bit resolution
		if ( _ain[i].curr > _ain[i].max) _ain[i].max = _ain[i].curr;
		if ( _ain[i].curr < _ain[i].min) _ain[i].min = _ain[i].curr;
        _ain[i].filtered = fir_filter (& _ain[i].fir, _ain[i].curr, discards[i]);
		_ain[i].scaled = _sensor_scale ( _ain[i].filtered, _ain[i].def_min, _ain[i].def_max);
	}

	unsigned char relays = 0;
	if (_ain[3].filtered < 0x800) 
		relays |= (1<<0);
	if (_ain[4].filtered < 0x800) 
		relays |= (1<<1);
	CAN_BUFFER buf = (CAN_BUFFER) { relays, _ain[0].scaled, _ain[1].scaled, _ain[2].scaled, 5, 6, 7, 8 };
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);
}


void main()
{
	lcd_initialize();

	//_intro ();

	exos_port_create(&_can_rx_port, NULL);
	exos_fifo_create(&_can_free_msgs, NULL);
	for(int i = 0; i < CAN_MSG_QUEUE; i++) exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

	hal_can_initialize(LCD_CAN_BUS, 250000);
	hal_fullcan_setup(_can_setup, NULL);
	hal_adc_initialize(1000, 16);

	unsigned int st_time_base = 0;
    unsigned int time_base = exos_timer_time();
	unsigned int prev_time = 0;

	int screen_count = 0;
    int initial_status = ST_DEBUG; //ST_LOGO_IN; //ST_DASH; //ST_DEBUG;
	int status =  initial_status;
	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start

	while(1)
	{
		// Read CAN messages from master 
		_get_can_messages ();

		if ( _dash.status & XCPU_STATE_ON)
		{
			if ((prev_cpu_state & XCPU_STATE_ON) == 0)
			{
				// Switch ON, Restart main loop
				time_base = exos_timer_time();
				prev_time, st_time_base = 0;
               	lcdcon_gpo_backlight(1);
                status =  initial_status;
				screen_count = 0;
                _fir_needs_init = 1;
			}

			unsigned int time = exos_timer_time();
			unsigned int req_update_time = 20;
			time -= time_base;
			unsigned int elapsed_time = time - prev_time;

			_read_send_analogic_inputs ();

			_clean_bilevel ( screen, 128, 64);


			int frame_skips = 0;
			if (( status > ST_INTRO_SECTION) && ( status < ST_INTRO_SECTION_END))
			{
				 frame_skips = 0;
				_intro ( &status, &st_time_base, time);
			}
			else
			{
				 frame_skips = 4;
				_runtime_screens ( status);
			}

			// Screen conversion & dump
			if ( screen_count > frame_skips)
			{
				_bilevel_linear_2_lcd ( scrrot, screen, 128, 64);
				lcd_dump_screen ( scrrot);
                screen_count=0;
			}
			screen_count++;

			if ( elapsed_time < req_update_time)
				exos_thread_sleep( req_update_time - elapsed_time);
			prev_time = time;
		} // dash status ON
		else
		{
			// DASH OFF
			exos_thread_sleep (50);

			if (prev_cpu_state & XCPU_STATE_ON) 
			{
				_clean_bilevel ( screen, 128, 64);
            	_bilevel_linear_2_lcd ( scrrot, screen, 128, 64);
				lcd_dump_screen ( scrrot);
				lcdcon_gpo_backlight(0);
			}
		}

        prev_cpu_state = _dash.status;
	}	// while (1)
}

#ifdef DEBUG
static int _lost_msgs = 0;
#endif

void hal_can_received_handler(int index, CAN_MSG *msg)
{ 
	XCPU_MSG *xmsg;
	switch(msg->EP.Id)
	{
		case 0x300:
			xmsg = (XCPU_MSG *)exos_fifo_dequeue(&_can_free_msgs);
			if (xmsg != NULL)
			{
				xmsg->CanMsg = *msg;
				exos_port_send_message(&_can_rx_port, (EXOS_MESSAGE *)xmsg);
			}
#ifdef DEBUG
			else _lost_msgs++;
#endif
			break;
	}
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

const static unsigned int _bmp_lock []  = 
{
0x0, 0x3ffffc00, 0x40000200, 0x40000200, 
0x40000200, 0x4f00f200, 0x4f80f200, 0x4fc0f200, 
0x4fe0f200, 0x4ff0f200, 0x4ff8f200, 0x4ffcf200, 
0x4f7ef200, 0x4f3ff200, 0x4f1ff200, 0x4f0ff200, 
0x4f07f200, 0x4f03f200, 0x4f01f200, 0x4f00f200, 
0x40000200, 0x40000200, 0x40000200, 0x3ffffc00, 
0x0
};

const static unsigned int _bmp_warning []  = 
{
0x0, 0x180000, 0x3c0000, 0x7c0000, 
0x6e0000, 0xe60000, 0xff0000, 0x1fb8000, 
0x3b98000, 0x339c000, 0x718e000, 0x6186000, 
0xe187000, 0x1c183800, 0x18003800, 0x38381c00, 
0x30380c00, 0x7fe00e00, 0x7fffff00, 0x7ffffe00, 
0x0
};

const static unsigned int _bmp_fatal_error [] = 
{ 
0x0, 0xffff00, 0xffff00, 0x1c000, 
0x1c000, 0xfff800, 0x3fffc00, 0x47800f80, 
0x6e000fbe, 0x6c0001be, 0x6c0001fe, 0x6c0001f6, 
0x6c000006, 0x7c000006, 0x7c000006, 0x6c000006, 
0x6c000006, 0x6c000006, 0x6e0001f6, 0x6fc001f6, 
0x47f001be, 0x3c01be, 0x1fff80, 0x7ff80
};

const static unsigned int _bmp_cruisin [] = 
{
0x0, 0x60000000, 0x7207f000, 0x3a1ffc00, 
0x1e7c9f00, 0xef08780, 0x3fc081c0, 0x3c081e0, 
0x3e00260, 0x7700470, 0x6380030, 0xe1c0038, 
0xc0fc018, 0xc076018, 0xfc221f8, 0xc036018, 
0xc01c018, 0xe000038, 0x6000030, 0x7000070, 
0x3080860, 0x39004e0, 0x1e003c0, 0xe00380, 
0x600300, 0x0
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
0xff00, 0x0, 0x0, 0x0, 
0x7ffe0, 0x0, 0x0, 0x60000000, 
0x7fff0, 0x0, 0x0, 0xf0000000, 
0x3fffc, 0x0, 0x0, 0xf0000000, 
0x61fffe, 0x0, 0x0, 0xf0000000, 
0xf001ff, 0x700e0c, 0x3830061, 0xff1e00f0, 
0x1f8007f, 0x80f81f1e, 0x7c780f3, 0xff9f01f0, 
0x1f8001f, 0x807c3e1e, 0xf8780f3, 0xff8f01e0, 
0x3f0000f, 0xc07c3e1e, 0x1f0780f1, 0xff0f83e0, 
0x7e03c07, 0xe03e7c1e, 0x3e0780f0, 0xf00f83e0, 
0x7e03f07, 0xe01e781e, 0x7c0780f0, 0xf00783c0, 
0x7c31f83, 0xe01ff81e, 0xf80780f0, 0xf007c7c0, 
0xfc38783, 0xf00ff01f, 0xf80780f0, 0xf003c7c0, 
0xf8783c1, 0xf007e01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfe0780f0, 0xf001ff80, 
0xf8701c1, 0xf01ff81f, 0xbf0781f0, 0xf001ff00, 
0xf8783c1, 0xf03e7c1f, 0x1f87c3f0, 0xf800ff00, 
0xf83c781, 0xf03c3c1e, 0xfc7fff0, 0xfcc0ff00, 
0xfc3ff83, 0xf07c3e1e, 0x7e7fff0, 0xffc0fe00, 
0x7c1ff03, 0xe0fc3f1e, 0x3e3fcf0, 0x7fc07e00, 
0x7e07c07, 0xe0781e0c, 0x1c1f860, 0x3f807e00, 
0x7e00007, 0xe0000000, 0x0, 0x3c00, 
0x3f0000f, 0xc0000000, 0x0, 0x3c00, 
0x3f8001f, 0x80020002, 0x2004120, 0x3c00, 
0x1fe007f, 0x80020002, 0x4020, 0x7800, 
0xff81ff, 0x3a1c77, 0x3a387124, 0x61c0f800, 
0x7ffffe, 0x4a2482, 0x22404928, 0x9207f000, 
0x3ffffc, 0xfa7c82, 0x22404930, 0xf3cfe000, 
0x1ffff0, 0x422082, 0x22404928, 0x804fc000, 
0x7ffe0, 0x399c71, 0xa2387124, 0x73878000, 
0xff00, 0x0, 0x0, 0x0, 
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
0x0, 0x0, 0x0, 0x0
};

static const unsigned int xkuty2_bw [] = 
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
0xff00, 0x0, 0x0, 0x0, 
0x7ffe0, 0x0, 0x0, 0xf0000000, 
0x7fff0, 0x0, 0x0, 0xf0000000, 
0x3fffc, 0x0, 0x0, 0xf0000000, 
0x61fffe, 0x0, 0x0, 0xf0000000, 
0xf001ff, 0x781e1e, 0x3c780f3, 0xff9e00f0, 
0x1f8007f, 0x80781e1e, 0x7c780f3, 0xff9f01f0, 
0x1f8001f, 0x807c3e1e, 0xf8780f3, 0xff8f01e0, 
0x3f0000f, 0xc07c3e1e, 0x1f0780f3, 0xff8f83e0, 
0x7e07c07, 0xe03e7c1e, 0x3e0780f0, 0xf00f83e0, 
0x7e03f07, 0xe01e781e, 0x7c0780f0, 0xf00783c0, 
0x7c31f83, 0xe01ff81e, 0xf80780f0, 0xf007c7c0, 
0xfc38783, 0xf00ff01f, 0xf80780f0, 0xf003c7c0, 
0xf8783c1, 0xf007e01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfe0780f0, 0xf001ff80, 
0xf8701c1, 0xf01ff81f, 0xbf0781f0, 0xf001ff00, 
0xf8783c1, 0xf03e7c1f, 0x1f87c3f0, 0xf800ff00, 
0xf83c781, 0xf03c3c1e, 0xfc7fff0, 0xfcc0ff00, 
0xfc3ff83, 0xf07c3e1e, 0x7e7fff0, 0xffc0fe00, 
0x7c1ff03, 0xe07c3e1e, 0x3e3fcf0, 0x7fc07e00, 
0x7e07c07, 0xe07c3e1e, 0x1e1f8f0, 0x3fc07e00, 
0x7e00007, 0xe0000000, 0x0, 0x3c00, 
0x3f0000f, 0xc0000000, 0x0, 0x3c00, 
0x3f8001f, 0x80002000, 0x40401048, 0x3c00, 
0x1fe007f, 0x80002000, 0x40001008, 0x7800, 
0xff81ff, 0x3a1ce, 0xe74e1c49, 0x31c0f800, 
0x7ffffe, 0x4a250, 0x4450124a, 0x4a0ff000, 
0x3ffffc, 0xfa7d0, 0x4450124c, 0x7bcfe000, 
0x1ffff0, 0x42210, 0x4450124a, 0x404fc000, 
0x7ffe0, 0x399ce, 0x344e1c49, 0x3b8f8000, 
0xff00, 0x0, 0x0, 0x0, 
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
0x0, 0x0, 0x0, 0x0
};

static const unsigned int exos_bw [] = 
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
0x0, 0x0, 0x80, 0x40000000, 
0x0, 0x0, 0x80, 0x40000000, 
0x0, 0x0, 0x80, 0x40000000, 
0xb807, 0x811103c1, 0x60f00e80, 0x5c104000, 
0xc408, 0x41110421, 0x81081180, 0x62088000, 
0x8210, 0x21290811, 0x2042080, 0x41088000, 
0x8210, 0x21290811, 0x2042080, 0x41088000, 
0x8210, 0x20aa0ff1, 0x3fc2080, 0x41050000, 
0x8210, 0x20aa0801, 0x2002080, 0x41050000, 
0x8210, 0x20aa0811, 0x2042080, 0x41050000, 
0xc408, 0x40440421, 0x1081180, 0x62050000, 
0xb807, 0x804403c1, 0xf00e80, 0x5c020000, 
0x8000, 0x0, 0x0, 0x20000, 
0x8000, 0x0, 0x0, 0x40000, 
0x8000, 0x0, 0x0, 0x180000, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0xff, 0xffc00003, 0xffff0000, 0x7f800000, 
0x3ff, 0xfff0000f, 0xffffc001, 0xffe00000, 
0x7ff, 0xfffc003f, 0xffffe003, 0xfff80000, 
0xfff, 0xfffe007f, 0xfffff007, 0xfffc0000, 
0x1fff, 0xffff00ff, 0xfffff80f, 0xfffc0000, 
0x3f80, 0x3f81fc, 0x3f81fc1f, 0xc0780000, 
0x7e00, 0x1fc3f8, 0x7f00fe3f, 0x80300000, 
0x7c00, 0xfe7f0, 0x7e007e3f, 0x0, 
0xf800, 0x7ffe0, 0xfc003f7e, 0x0, 
0xf800, 0x3ffc0, 0xf8001f7e, 0x0, 
0xffff, 0xfe01ff80, 0xf8001f7f, 0xffff8000, 
0xffff, 0xfc00ff00, 0xf8001f7f, 0xffff8000, 
0xffff, 0xf800ff00, 0xf8001f7f, 0xffff8000, 
0xffff, 0xf001ff80, 0xf8001f3f, 0xffff8000, 
0xf800, 0x3ffc0, 0xf8001f00, 0x1f8000, 
0xf800, 0x7ffe0, 0xfc003f00, 0x1f8000, 
0x7c00, 0xfe7f0, 0x7e007e00, 0x1f8000, 
0x7e00, 0x1fc3f8, 0x7f00fe00, 0x3f8000, 
0x3f80, 0x3f81fc, 0x3f81fc00, 0x7f0000, 
0x1fff, 0xffff00ff, 0xffffffff, 0xffff0000, 
0xfff, 0xfffe007f, 0xffffffff, 0xfffe0000, 
0x7ff, 0xfffc003f, 0xffffffff, 0xfffc0000, 
0x3ff, 0xfff0000f, 0xffffffff, 0xfff80000, 
0xff, 0xffc00003, 0xffffffff, 0xffe00000, 
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
