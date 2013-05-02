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
#include "xkuty_gfx.h"
#include "event_recording.h"


#define DISPW (128)
#define DISPH (64)

static unsigned char scrrot [(DISPW*DISPH)/8];
static unsigned int  screen [(DISPW*DISPH)/32];

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h);
static void _clean_bilevel ( unsigned int* scr, int w, int h)  { for (int i=0; i<((w*h)/32); i++) scr [i] = 0;  }

static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };

typedef struct {
	int status;
	int speed;
	int distance_hi;
	int distance_lo;
	int battery_level_fx8;
    int speed_adjust;
} DASH_DATA;

static DASH_DATA _dash = {0,0,0,0,0,0};

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
	ST_DEBUG_INPUT,
    ST_DEBUG_SPEED,
};

static inline void _limit ( int* v, int min, int max)
{
    if ( *v < min) *v = min;
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
    for (y=0; y<DISPH; y++)
	{
        for ( x=0; x<DISPW; x+=32)
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

static inline int _sensor_scale ( int in, int base, int top, int magnitude)
{
	int len = top - base;
	int pos = in  - base;
	_limit ( &pos, 0, len);
	int res = (pos * magnitude) / len;
	_limit ( &res, 0, magnitude - 1);
	return res;
}

static char _tmp [20];

static void _draw_text ( const char* text, const MONO_SPR* font, int x, int y)
{ 
	MONO_SPR spr = *font;
	int i = 0;
	while ( text[i])
	{
		int glyph_w = font->w;
		int glyph = -1;
		if (( text [i]>='0' && text [i]<='9'))
			glyph = text [i] - '0';
		else
			switch ( text [i])	// Hyper-hack
			{
				case '.': glyph = 10, glyph_w = font->w >> 1; break;
				case '-': glyph = 11; break;
			}
		if ( glyph != -1)
		{
			spr.bitmap = font->bitmap + (spr.stride_bitmap * spr.h * glyph);
			mono_draw_sprite ( screen, DISPW, DISPH, &spr, x, y);
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
		mono_draw_sprite ( screen, DISPW, DISPH, &spr, x, y);

	spr = *spr0;
	spr.bitmap = spr0->bitmap + (spr.stride_bitmap * cut_y);
	spr.h      = spr.h - cut_y;
	if ( spr.h > 0)
		mono_draw_sprite ( screen, DISPW, DISPH, &spr, x, y + cut_y);
}

static void _horizontal_sprite_comb ( const MONO_SPR* spr0, const MONO_SPR* spr1, 
                                      int level_fx8, int x, int y)
{
	static unsigned int show_mask[] = {0,0,0,0};
	_limit ( &level_fx8, 0,0x100);
	int cut_x = (level_fx8 * spr0->w) >> 8;
	int spans = cut_x >> 5;
	int inter = cut_x & 0x1f;
	int i=0;
	for ( ;i<spans;i++)
		show_mask[i] = 0xffffffff;
	show_mask[i] = (inter == 0) ? 0 : (unsigned int)(-1<<(32-inter));
	mono_draw_sprite ( screen, DISPW, DISPH, spr1, x, y);
	MONO_SPR spr = *spr0;
	spr.mask = show_mask;
	spr.stride_mask = 0;
	mono_draw_sprite ( screen, DISPW, DISPH, &spr, x, y);
}

static int _fir_needs_init = 1;

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
	{0,0,0,0xffff, 587, 3109},	// 0 - Throtle  (16 bits)   2400->maximum accepted by motor controller, 1050->minimum 
	{0,0,0,0xffff, 1765,2400},	// 1 - Brake left (16 bits)
	{0,0,0,0xffff, 1900,2600},	// 2 - Brake right (16 bits)
	{0,0,0,0xffff, 0,4095},		// 3 - Cruise (bool)
	{0,0,0,0xffff, 0,4095},		// 4 - Horn  (bool)
};

#define THROTTLE_MASK    (1<<0)
#define BRAKE_LEFT_MASK  (1<<1)
#define BRAKE_RIGHT_MASK (1<<2)
#define CRUISE_MASK      (1<<3)
#define HORN_MASK        (1<<4)

static unsigned int _input_status = 0;
static char _latch_adj_up=0, _latch_adj_down=0, _latch_adj_metrics=0;

/*static const EVREC_CHECK _maintenance_screen_access[]=
{
	{BRAKE_LEFT_MASK,                 CHECK_PRESS},
	{THROTTLE_MASK,                   CHECK_PRESS},
	{BRAKE_LEFT_MASK | THROTTLE_MASK, CHECK_PRESSED},
	{BRAKE_LEFT_MASK | THROTTLE_MASK, CHECK_RELEASED},
	{HORN_MASK,                       CHECK_PRESS},	
	{0x00000000,CHECK_END},
};*/

static const EVREC_CHECK _maintenance_screen_access[]=
{
	{BRAKE_RIGHT_MASK | HORN_MASK | CRUISE_MASK, CHECK_PRESSED},
	{BRAKE_RIGHT_MASK | HORN_MASK | CRUISE_MASK, CHECK_RELEASED},
	{0x00000000,CHECK_END},
};

static void _read_send_analogic_inputs ()
{
	if ( _fir_needs_init)
	{
		_fir_needs_init = 0;
		fir_init( &_ain[0].fir, 6,  1, 500, 1);
		fir_init( &_ain[1].fir, 10, 1, 100, 3);
		fir_init( &_ain[2].fir, 10, 1, 100, 5);
		fir_init( &_ain[3].fir, 8, 0, 0, 0);
		fir_init( &_ain[4].fir, 8, 0, 0, 0);
	}

	for(int i=0; i<NUM_ADC_INPUTS; i++)
	{
		_ain[i].curr   = hal_adc_read(i) >> 4;	// 12 bit resolution
		if ( _ain[i].curr > _ain[i].max) _ain[i].max = _ain[i].curr;
		if ( _ain[i].curr < _ain[i].min) _ain[i].min = _ain[i].curr;
        _ain[i].filtered = fir_filter (& _ain[i].fir, _ain[i].curr);
		_ain[i].scaled = _sensor_scale ( _ain[i].filtered, _ain[i].def_min, _ain[i].def_max, 0xfff);
	}

	unsigned char relays = 0;
	if (_ain[3].filtered < 0x800) 
		relays |= XCPU_RELAY_CRUISIN;
	if (_ain[4].filtered < 0x800) 
		relays |= XCPU_RELAY_HORN;
	if ( _latch_adj_up)
		relays |= XCPU_RELAY_ADJUST_UP, _latch_adj_up=0;
	if ( _latch_adj_down)
		relays |= XCPU_RELAY_ADJUST_DOWN, _latch_adj_down=0;
	if ( _latch_adj_metrics)
		relays |=XCPU_RELAY_CHANGE_METRICS, _latch_adj_metrics=0;

	CAN_BUFFER buf = (CAN_BUFFER) { relays, _ain[0].scaled>>4, _ain[1].scaled>>4,
									_ain[2].scaled>>4, 5, 6, 7, 8 };
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);

	// Record inputs for sequence triggering (to start debug services)
	 _input_status = (((_ain[0].scaled & 0x80) >> 7) << 0) |
					(((_ain[1].scaled & 0x80) >> 7) << 1) |
					(((_ain[2].scaled & 0x80) >> 7) << 2) |
					(relays << 3);
	event_record ( _input_status);
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
				_dash.speed             = xmsg->MasterInfo.speed;
				_dash.battery_level_fx8 = xmsg->MasterInfo.battery_level_fx8;
				_dash.status			= xmsg->MasterInfo.status;
                _dash.speed_adjust      = xmsg->MasterInfo.speed_adjust;
				_dash.distance_hi       = xmsg->MasterInfo.distance / 10;
				_dash.distance_lo       = xmsg->MasterInfo.distance % 10;
				break;
		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	}
}


#define INTRO_BMP1 _xkuty2_bw
#define INTRO_BMP2 _exos_bw

static EXOS_TIMER _timer_update;	// Could be local?

static void _intro ( int* status, int* st_time_base, int time)
{
	int factor;
	const int cx = DISPW>>1, cy=DISPH>>1;
	const int logo_time = 600; // ms
	switch ( *status)
	{
		case ST_LOGO_IN:
			factor = time/6;
			_xkuty_effect ( INTRO_BMP1, factor * factor, cx, cy);
			if ( factor > 75) 
				*status = ST_LOGO_SHOW, *st_time_base = time;
			break;
		case ST_LOGO_SHOW:
			_xkuty_effect ( INTRO_BMP1, 100*100, cx, cy);
			if ( time >= (*st_time_base+logo_time)) 
				*status = ST_LOGO_OUT, *st_time_base = time;
			break;
		case ST_LOGO_OUT:
			factor = 75-((time - *st_time_base) / 6);
			_xkuty_effect ( INTRO_BMP1, factor * factor, cx, cy);
			if ( factor <= 0) 
				*status = ST_EXOS_IN, *st_time_base = time;
			break;
		case ST_EXOS_IN:
			factor = ((time - *st_time_base)/3);
			_xkuty_effect ( INTRO_BMP2, factor * factor, 0, DISPH);
			if ( factor > 140) 
				*status = ST_EXOS_SHOW, *st_time_base = time;
			break;
		case ST_EXOS_SHOW:
			_xkuty_effect ( INTRO_BMP2, 150*150, 0, DISPH);
			if ( time >= (*st_time_base+logo_time)) 
				*status = ST_EXOS_OUT, *st_time_base = time;
			break;
		case ST_EXOS_OUT:
			factor = 140-((time - *st_time_base)/3);
			_xkuty_effect ( INTRO_BMP2, factor * factor, DISPW, DISPH);
			if ( factor <= 0) 
				*status = ST_DASH;
			break;
	}
}

static int frame_dumps = 0;

#define POS_BATTERY_BAR   5,   4
#define POS_NEUTRAL       98,  7
#define POS_FATAL         80,  12
#define POS_SPEED_TEXT    124, 9
#define POS_DISTANCE_TEXT 130, 50
#define POS_CRUISE        36,  8
#define POS_WARNING       37,  28
#define POS_KMH           100, 4
#define POS_KM            108, 41

static void _runtime_screens ( int* status)
{
	// General runtime switch; insert new screens here
	switch ( *status)
	{
		case ST_DASH:
			{
				//_dash.status |= XCPU_STATE_CRUISE_ON;
				//_dash.status |= XCPU_STATE_WARNING;
                //_dash.status |= XCPU_STATE_ERROR;
				int l;
				l = sprintf ( _tmp, "%d", _dash.speed);
				if ( _dash.status & (XCPU_STATE_NEUTRAL | XCPU_STATE_ERROR))
				{
					if ( _dash.status & XCPU_STATE_NEUTRAL)
						mono_draw_sprite ( screen, DISPW, DISPH, &_lock_spr, POS_NEUTRAL);
					if (( _dash.status & XCPU_STATE_ERROR) && ((frame_dumps & 8) == 0))
						mono_draw_sprite ( screen, DISPW, DISPH, &_fatal_error_spr, POS_FATAL);
				}
				else
					_draw_text ( _tmp, &_font_spr_big,  - (24*l) + POS_SPEED_TEXT);

				l = sprintf ( _tmp, "%d", _dash.distance_hi);
				_tmp[l] = '.', l++;
				l += sprintf ( &_tmp[l], "%d", _dash.distance_lo);
				_draw_text ( _tmp, &_font_spr_small, - (_font_spr_small.w * l) + POS_DISTANCE_TEXT);

                _vertical_sprite_comb ( &_battery_full, &_battery_empty, 0x100 - _dash.battery_level_fx8, POS_BATTERY_BAR);

				if ( _dash.status & XCPU_STATE_CRUISE_ON)
					mono_draw_sprite ( screen, DISPW, DISPH, &_cruisin_spr, POS_CRUISE);
				if ( _dash.status & XCPU_STATE_WARNING)
					mono_draw_sprite ( screen, DISPW, DISPH, &_warning_spr, POS_WARNING);

				//mono_draw_sprite ( screen, DISPW, DISPH, &_kmh_spr, POS_KMH);
				mono_draw_sprite ( screen, DISPW, DISPH, 
									(_dash.status & XCPU_STATE_MILES) ? &_mi_spr : &_km_spr, 
									POS_KM);

				if ( _dash.speed == 0)
					if ( event_happening ( _maintenance_screen_access, 50)) // 1 second
						*status = ST_DEBUG_SPEED;
				break;
			}
			break;

		case ST_DEBUG_INPUT:
			for (int i=0; i<NUM_ADC_INPUTS; i++)
			{
				int y=12*i;
				/*if(( frame>>4) & 1)
					sprintf ( _tmp, "%d.%d %d", i, _ain[i].curr,_ain[i].max);
				else
					sprintf ( _tmp, "%d.%d %d", i, _ain[i].curr,_ain[i].min);*/
				sprintf ( _tmp, "%d.%d %d", i, _ain[i].filtered, _ain[i].scaled >> 4);
				_draw_text ( _tmp, &_font_spr_small, 0, y);
			}
			break;
		case ST_DEBUG_SPEED:
			{
				const EVREC_CHECK speed_adj_down[]= {{BRAKE_LEFT_MASK, CHECK_PRESS},{0x00000000,CHECK_END}};
				const EVREC_CHECK speed_adj_up[]  = {{BRAKE_RIGHT_MASK, CHECK_PRESS},{0x00000000,CHECK_END}};
				const EVREC_CHECK speed_adj_exit[]= {{HORN_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				const EVREC_CHECK speed_adj_metrics[]= {{CRUISE_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				if ( event_happening ( speed_adj_down,1))
					_latch_adj_down=1;
				if ( event_happening ( speed_adj_up,1))
					_latch_adj_up=1;
				if ( event_happening ( speed_adj_metrics,1))
					_latch_adj_metrics=1;

				_limit( &_dash.speed_adjust, -10, 10);
				int bar = 0x80 + (_dash.speed_adjust * (0x80/10));
				mono_draw_sprite ( screen, DISPW, DISPH, &_speed_adjust_spr, 16, 2);
                //mono_draw_sprite ( screen, DISPW, DISPH, &_xkuty_pic_spr, -100, 2);
				sprintf ( _tmp, "%d", _dash.speed_adjust);
				_draw_text ( _tmp, &_font_spr_big, 36, 20);
				mono_draw_sprite ( screen, DISPW, DISPH, 
									(_dash.status & XCPU_STATE_MILES) ? &_mi_spr : &_km_spr, 
									100, 32);
				_horizontal_sprite_comb ( &_adjust_full_spr, &_adjust_empty_spr, bar, 40,46); 
                if ( event_happening ( speed_adj_exit,1))
					*status = ST_DASH;
			}
			break;
	}
}


void main()
{
	lcd_initialize();

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
    int initial_status = ST_LOGO_IN; //ST_LOGO_IN; //ST_DASH; //ST_DEBUG_INPUT; // ST_DEBUG_SPEED
	int status =  initial_status;
	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start
	_dash.status |= XCPU_STATE_ON;
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

			_clean_bilevel ( screen, DISPW, DISPH);


			int frame_skips = 0;
			if (( status > ST_INTRO_SECTION) && ( status < ST_INTRO_SECTION_END))
			{
				 frame_skips = 0;
				_intro ( &status, &st_time_base, time);
			}
			else
			{
				 frame_skips = 4;
				_runtime_screens ( &status);
			}

			// Screen conversion & dump
			if ( screen_count > frame_skips)
			{
				_bilevel_linear_2_lcd ( scrrot, screen, DISPW, DISPH);
				lcd_dump_screen ( scrrot);
                screen_count=0;
				frame_dumps++;
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
				_clean_bilevel ( screen, DISPW, DISPH);
            	_bilevel_linear_2_lcd ( scrrot, screen, DISPW, DISPH);
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

