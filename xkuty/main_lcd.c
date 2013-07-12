#include <support/lcd/lcd.h>
#include <support/adc_hal.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <support/can_hal.h>
#include <modules/gfx/mono.h>
#include <modules/gfx/font.h>
#include <stdio.h>
#include <assert.h>

#include "xcpu.h"
#include "fir.h"
#include "xkuty_gfx.h"
#include "event_recording.h"
#include "xcpu/throttle_curves.h"

#include "small_font.h"

#define DISPW (128)
#define DISPH (64)

static unsigned int  _screen_pixels [(DISPW*DISPH)/32];

static const CANVAS _screen = 
{ 
	(unsigned char*)_screen_pixels,
    DISPW, DISPH,
    DISPW/8,
    PIX_1_MONOCHROME
};

#define MAIN_LOOP_TIME  (20)
#define RESEND_TIMES    (25)

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static void _bilevel_linear_2_lcd ( unsigned char* dst, unsigned int* src, int w, int h);
static void _clean_bilevel ( const CANVAS* scr)  
{ 
	unsigned int* pix = (unsigned int*)scr->pixels;
	int words = (scr->stride_bytes * scr->h) >> 2;
	for (int i=0; i<words; i++) 
		pix [i] = 0;  
}

static const CAN_EP _eps[] = {{0x300, LCD_CAN_BUS}, {0x301, LCD_CAN_BUS} };

typedef struct {
	int status;
	int speed;
	int distance_hi;
	int distance_lo;
	int battery_level_fx8;
    int speed_adjust;
	int drive_mode;
} DASH_DATA;

static DASH_DATA _dash = {0,0,0,0,0,0, CURVE_SOFT};

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
	ST_FACTORY_MENU,
    ST_ADJUST_WHEEL_DIA,
	ST_ADJUST_THROTTLE_MAX,
	ST_ADJUST_THROTTLE_MIN,
	ST_ADJUST_DRIVE_MODE,
};

static inline int _xkutybit ( int x, int y, int t, int cx, int cy)
{
	int xx = x - cx; int yy = y - cy;
	int r = xx*xx + yy*yy;
	return ( r < t) ? 1 : 0;
}

void _xkuty_effect ( const unsigned int* bitmap, int t, int cx, int cy)
{
	unsigned int* pixels = (unsigned int *)_screen.pixels;
	if ( t<0) t=0x7fffffff;		// Show the full bitmap
    int i=0, x=0, y=0, sx=0;
    for (y=0; y<_screen.h; y++)
	{
        for ( x=0; x<_screen.w; x+=32)
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

static inline int _sensor_scale ( int in, int base, int top, int magnitude)
{
	int len = top - base;
	int pos = in  - base;
	pos = __LIMIT( pos, 0, len);
	int res = (pos * magnitude) / len;
	res = __LIMIT( res, 0, magnitude - 1);
	return res;
}

static char _tmp [20];

static void _draw_text ( const char* text, const SPRITE* font, int x, int y)
{ 
	SPRITE spr = *font;
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
				case '.': glyph = 10, glyph_w = (font->w >> 1)+1; break;
				case '-': glyph = 11; break;
			}
		if ( glyph != -1)
		{
			spr.bitmap = font->bitmap + (spr.stride_bitmap * spr.h * glyph);
			mono_draw_sprite ( &_screen, &spr, x, y);
		}
		x += glyph_w;
		i++;
	}
}


static void _vertical_sprite_comb ( const SPRITE* spr0, const SPRITE* spr1, 
                                    int level_fx8, int x, int y)
{
	SPRITE spr = *spr1;
	level_fx8 = __LIMIT ( level_fx8, 0,0x100);
	int cut_y = (level_fx8 * spr.h) >> 8;
	spr.h     = cut_y;
	if ( spr.h > 0)
		mono_draw_sprite ( &_screen, &spr, x, y);

	spr = *spr0;
	spr.bitmap = spr0->bitmap + (spr.stride_bitmap * cut_y);
	spr.h      = spr.h - cut_y;
	if ( spr.h > 0)
		mono_draw_sprite ( &_screen, &spr, x, y + cut_y);
}

static void _horizontal_sprite_comb ( const SPRITE* spr0, const SPRITE* spr1, 
                                      int level_fx8, int x, int y)
{
	static unsigned int show_mask[] = {0,0,0,0};

	mono_draw_sprite ( &_screen, spr1, x, y);

	level_fx8 = __LIMIT( level_fx8, 0,0x100);
	int cut_x = (level_fx8 * spr0->w) >> 8;
	int spans = cut_x >> 5;
	int inter = cut_x & 0x1f;
	int i=0;
	for ( ;i<4;i++)
		if ( i < (cut_x >> 5)) 
			show_mask[i] = 0xffffffff;
		else
			if (( cut_x>>5) == i)
				show_mask[i] = (unsigned int)(-1<<(32-inter));
			else
				show_mask[i] = 0;
	SPRITE spr = *spr0;
	spr.mask = show_mask;
	spr.stride_mask = 0;
	mono_draw_sprite ( &_screen, &spr, x, y);
}

static void _print_small ( const char* str, int x, int y)
{
	int t = font_calc_len ( &font_small, str, FONT_PROPORTIONAL | FONT_KERNING);
	int xx = 64-(t/2);
	if ( x != -1)
		xx = x;
	font_draw ( &_screen, str, &font_small, FONT_PROPORTIONAL | FONT_KERNING, xx, y);
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
	{0,0,0,0xffff, 0,   4095},	// 3 - Cruise (bool)
	{0,0,0,0xffff, 0,   4095},	// 4 - Horn  (bool)
};

#define THROTTLE_MASK    (1<<0)
#define BRAKE_LEFT_MASK  (1<<1)
#define BRAKE_RIGHT_MASK (1<<2)
#define CRUISE_MASK      (1<<3)
#define HORN_MASK        (1<<4)

#define THROTTLE_IDX     (0)
#define BRAKE_LEFT_IDX   (1)
#define BRAKE_RIGHT_IDX  (2)
#define CRUISE_IDX       (3)
#define HORN_IDX         (4)

static unsigned int _input_status = 0;
static char _adj_up=0, _adj_down=0;
static char _adj_throttle=0;
static unsigned short _adj_throttle_max=0, _adj_throttle_min=0;
static char _adj_drive_mode=0, _adj_drive_mode_cnt=0;
static char _switch_units_cnt=0, _lights_off_cnt=0;

static const EVREC_CHECK _maintenance_screen_access[]=
{
	{BRAKE_RIGHT_MASK | HORN_MASK | CRUISE_MASK, CHECK_PRESSED},
	{BRAKE_RIGHT_MASK | HORN_MASK | CRUISE_MASK, CHECK_RELEASED},
	{0x00000000,CHECK_END},
};

static const EVREC_CHECK _mode_screen_access[]=
{
	{CRUISE_MASK | HORN_MASK, CHECK_RELEASED},
	{CRUISE_MASK | HORN_MASK, CHECK_PRESSED},
	{CRUISE_MASK | HORN_MASK, CHECK_RELEASED},
	{CRUISE_MASK | HORN_MASK, CHECK_PRESSED},
	{CRUISE_MASK | HORN_MASK, CHECK_RELEASED},
	{0x00000000,CHECK_END},
};

static void _read_send_analogic_inputs ( int status)
{
	if ( _fir_needs_init)
	{
		_fir_needs_init = 0;
		#if 0
		fir_init( &_ain[THROTTLE_IDX].fir,    6,  1, 500, 1);
		fir_init( &_ain[BRAKE_LEFT_IDX].fir,  10, 1, 100, 3);
		fir_init( &_ain[BRAKE_RIGHT_IDX].fir, 10, 1, 100, 5);
		fir_init( &_ain[CRUISE_IDX].fir,      8, 0, 0, 0);
		fir_init( &_ain[HORN_IDX].fir,        8, 0, 0, 0);
		#else
		fir_init( &_ain[THROTTLE_IDX].fir,    6, 1, 500, 2);
		fir_init( &_ain[BRAKE_LEFT_IDX].fir,  6, 1, 100, 2);
		fir_init( &_ain[BRAKE_RIGHT_IDX].fir, 6, 1, 100, 2);
		fir_init( &_ain[CRUISE_IDX].fir,      6, 0, 0, 0);
		fir_init( &_ain[HORN_IDX].fir,        6, 0, 0, 0);
		#endif
	}

	for(int i=0; i<NUM_ADC_INPUTS; i++)
	{
		_ain[i].curr   = hal_adc_read(i) >> 4;	// 12 bit resolution
        _ain[i].filtered = fir_filter (& _ain[i].fir, _ain[i].curr);
		_ain[i].scaled = _sensor_scale ( _ain[i].filtered, _ain[i].def_min, _ain[i].def_max, 0xfff);
		int tocmp = _ain[i].filtered;
		if ( tocmp > _ain[i].max) _ain[i].max = tocmp;
		if ( tocmp < _ain[i].min) _ain[i].min = tocmp;
	}

	unsigned short relays = 0;
	if (_ain[CRUISE_IDX].filtered < 0x800) 
		relays |= XCPU_BUTTON_CRUISE;
	if (_ain[HORN_IDX].filtered < 0x800) 
		relays |= XCPU_BUTTON_HORN;
	if ( _adj_up)
		relays |= XCPU_BUTTON_ADJUST_UP;
	if ( _adj_down)
		relays |= XCPU_BUTTON_ADJUST_DOWN;
	if ( _switch_units_cnt > 0)
		_switch_units_cnt--,
		relays |= XCPU_BUTTON_SWITCH_UNITS;
	if ( _adj_throttle > 0)
        relays |= XCPU_BUTTON_ADJ_THROTTLE, _adj_throttle--;
	if ( _adj_drive_mode_cnt > 0)
		_adj_drive_mode_cnt--,
		relays |= _adj_drive_mode << XCPU_BUTTON_ADJ_DRIVE_MODE_SHIFT;
	if ( _lights_off_cnt > 0)
		_lights_off_cnt--, 
        relays |= XCPU_BUTTON_LIGHTS_OFF;

	// Record inputs for sequence triggering (to start debug services)
	 _input_status = (((_ain[THROTTLE_IDX].scaled & 0x80) >> 7) << 0) |
					(((_ain[BRAKE_LEFT_IDX].scaled & 0x80) >> 7) << 1) |
					(((_ain[BRAKE_RIGHT_IDX].scaled & 0x80) >> 7) << 2) |
					(relays << 3);
	event_record ( _input_status);

	unsigned int throttle = _ain[THROTTLE_IDX].scaled;
	// Throttle only should work on dash screen, the rest are debug or intro screens
	// Same with horn
	if ( status != ST_DASH)
	{
		relays &= ~XCPU_BUTTON_HORN;
		throttle = 0;
	}

	CAN_BUFFER buf = (CAN_BUFFER) { relays, 
									throttle & 0xff, // Throttle low
									throttle >> 8,	// Throttle high
									_ain[BRAKE_LEFT_IDX].scaled >> 4,
									_ain[BRAKE_RIGHT_IDX].scaled >> 4, 
									_adj_throttle_min >> 4, 
									_adj_throttle_max >> 4,
									relays >> 8};
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = LCD_CAN_BUS }, &buf, 8, CANF_PRI_ANY);
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
			{
				XCPU_MASTER_OUT1* tmsg = (XCPU_MASTER_OUT1*)&xmsg->CanMsg.Data.u8[0];
				_dash.speed             = tmsg->speed;
				_dash.battery_level_fx8 = tmsg->battery_level_fx8;
				_dash.status			= tmsg->status;
                _dash.speed_adjust      = tmsg->speed_adjust;
				_dash.distance_hi       = tmsg->distance / 10;
				_dash.distance_lo       = tmsg->distance % 10;
			}
			break;

			case 0x301:
			{
				XCPU_MASTER_OUT2* tmsg = (XCPU_MASTER_OUT2*)&xmsg->CanMsg.Data.u8[0];
				_ain[THROTTLE_IDX].def_min = tmsg->throttle_adj_min << 4;
				_ain[THROTTLE_IDX].def_max = tmsg->throttle_adj_max << 4;
				_dash.drive_mode           = tmsg->drive_mode;
			}
			break;
		}
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
        xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	}
}



static EXOS_TIMER _timer_update;
static int        _frame_dumps = 0;

static void _intro ( int* status, int* st_time_base, int time)
{
	int factor;
	const int cx = _screen.w>>1, cy=_screen.h>>1;
	const int logo_time = 600; // ms
	switch ( *status)
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
			_xkuty_effect ( _xkuty2_bw, factor * factor, cx, cy);
			if ( factor <= 0) 
				*status = ST_EXOS_IN, *st_time_base = time;
			break;
		case ST_EXOS_IN:
			factor = ((time - *st_time_base)/3);
			_xkuty_effect ( _exos_bw, factor * factor, 0, DISPH);
			if ( factor > 140) 
				*status = ST_EXOS_SHOW, *st_time_base = time;
			break;
		case ST_EXOS_SHOW:
			_xkuty_effect ( _exos_bw, -1, 0, DISPH);
			if ( time >= (*st_time_base+logo_time)) 
				*status = ST_EXOS_OUT, *st_time_base = time;
			break;
		case ST_EXOS_OUT:
			factor = 140-((time - *st_time_base)/3);
			_xkuty_effect ( _exos_bw, factor * factor, _screen.w, _screen.h);
			if ( factor <= 0) 
				*status = ST_DASH;
			break;
	}
}


#define POS_BATTERY_BAR   5,   4
#define POS_NEUTRAL       98,  7
#define POS_FATAL         80,  12
#define POS_SPEED_TEXT    124, 9
#define POS_DISTANCE_TEXT 130, 50
#define POS_CRUISE        36,  8
#define POS_WARNING       37,  28
#define POS_KMH           100, 4
#define POS_KM            108, 41
#define POS_MI            102, 37

#define POS_ADJUST_MSG    16, 2
#define POS_ADJUST_BAR    40, 46
#define POS_ADJUST_SPEED  36, 20

static int _adjust_creen ( int status, char* str, int next, unsigned short* res)
{
	const EVREC_CHECK throttle_adj_exit[]= {{CRUISE_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};

	_print_small ( str, -1, 14);
	sprintf ( _tmp, "%d", _ain[THROTTLE_IDX].curr >> 4);
	_draw_text ( _tmp, &_font_spr_small, 48, 26);

	_horizontal_sprite_comb ( &_adjust_full_spr, &_adjust_empty_spr, 
							_ain[THROTTLE_IDX].curr >> 4, POS_ADJUST_BAR); 
	if ( event_happening ( throttle_adj_exit,1))
	{
		*res = _ain[THROTTLE_IDX].curr;
		if ( next == ST_DASH)
			_adj_throttle = RESEND_TIMES;
		return next;
	}
	return status;
}

static int _fact_menu_mode = 0;

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
				int l = sprintf ( _tmp, "%d", _dash.speed);
				if ( _dash.status & (XCPU_STATE_NEUTRAL | XCPU_STATE_ERROR))
				{
					if ( _dash.status & XCPU_STATE_NEUTRAL)
						mono_draw_sprite ( &_screen, &_lock_spr, POS_NEUTRAL);
					if (( _dash.status & XCPU_STATE_ERROR) && ((_frame_dumps & 8) == 0))
						mono_draw_sprite ( &_screen, &_fatal_error_spr, POS_FATAL);
				}
				else
					_draw_text ( _tmp, &_font_spr_big,  - (24*l) + POS_SPEED_TEXT);

				l = sprintf ( _tmp, "%d", _dash.distance_hi);
				_tmp[l] = '.', l++;
				l += sprintf ( &_tmp[l], "%d", _dash.distance_lo);
				_draw_text ( _tmp, &_font_spr_small, - (_font_spr_small.w * l) + POS_DISTANCE_TEXT);

                _vertical_sprite_comb ( &_battery_full, &_battery_empty, 0x100 - _dash.battery_level_fx8, POS_BATTERY_BAR);

				if ( _dash.status & XCPU_STATE_CRUISE_ON)
					mono_draw_sprite ( &_screen, &_cruisin_spr, POS_CRUISE);
				if ( _dash.status & XCPU_STATE_WARNING)
					mono_draw_sprite ( &_screen, &_warning_spr, POS_WARNING);

				//mono_draw_sprite ( &screen, &_kmh_spr, POS_KMH);
                if (_dash.status & XCPU_STATE_MILES)
					mono_draw_sprite ( &_screen, &_mi_spr, POS_MI);
				else
					mono_draw_sprite ( &_screen, &_km_spr, POS_KM);
				if (( _dash.speed == 0) && ( _dash.status & XCPU_STATE_NEUTRAL))
                {
					if ( event_happening ( _maintenance_screen_access, 50)) // 1 second
						*status = ST_FACTORY_MENU;	
					if ( event_happening ( _mode_screen_access, 100)) // 2 second
                    {
						_adj_drive_mode = _dash.drive_mode - 1;
						_adj_drive_mode = __LIMIT( _adj_drive_mode, 0, 2);
						*status = ST_ADJUST_DRIVE_MODE;
					}
				}	
			}
			break;

		case ST_DEBUG_INPUT:
			{
				int fact = 100;	// Transform 12 fixed point to "%"
				int j, vv[5];
				char* sensor_names[] = {"Throttle", "Brk. lf.", "Brk. rt.", "Cruise", "Horn"};
				for (int i=0; i<NUM_ADC_INPUTS; i++)
				{
					int y=10 + 12*i;
					vv[0] = _ain[i].curr; 
					vv[1] = _ain[i].filtered;
					vv[2] = _ain[i].scaled;
					vv[3] = _ain[i].min;
					vv[4] = _ain[i].max;
					_print_small ( sensor_names[i], 0, y);
					for (j=0; j<5; j++)
					{
						sprintf ( _tmp, "%d%%", (vv[j] * fact) >> 12);
						_print_small ( _tmp, 36+j*20, y);
					}
				}
				if (( _input_status & HORN_MASK) && ( _input_status & CRUISE_MASK))
					*status = ST_DASH;
			}
			break;
		case ST_ADJUST_WHEEL_DIA:
			{
				const EVREC_CHECK speed_adj_exit[]= {{CRUISE_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				_adj_down = _adj_up = 0;
				if ( _input_status & BRAKE_LEFT_MASK)
					_adj_down=1;
				if ( _input_status & BRAKE_RIGHT_MASK)
					_adj_up=1;

				_dash.speed_adjust = __LIMIT( _dash.speed_adjust, -10, 10);
				int bar = 0x80 + (_dash.speed_adjust * (0x80/10));
				mono_draw_sprite ( &_screen, &_speed_adjust_spr, POS_ADJUST_MSG);
				sprintf ( _tmp, "%d", _dash.speed_adjust);
				_draw_text ( _tmp, &_font_spr_big, POS_ADJUST_SPEED);
									
				_horizontal_sprite_comb ( &_adjust_full_spr, &_adjust_empty_spr, bar, POS_ADJUST_BAR); 
                if ( event_happening ( speed_adj_exit,1))
					*status = ST_DASH;
			}
			break;
		case ST_ADJUST_THROTTLE_MAX:
            *status = _adjust_creen ( *status, "Push max throttle", ST_ADJUST_THROTTLE_MIN, 
										&_adj_throttle_max);
			break;		
		case ST_ADJUST_THROTTLE_MIN:
            *status = _adjust_creen ( *status, "Release throttle", ST_DASH, 
										&_adj_throttle_min);
			break;

		case ST_FACTORY_MENU:
			{
				const char _hei [] = { 22, 31, 40, 49, 58 };
				const EVREC_CHECK fact_menu_exit[]= {{HORN_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				const EVREC_CHECK menu_move[]= {{BRAKE_RIGHT_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				const EVREC_CHECK menu_press[]= {{CRUISE_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				int anm = (_frame_dumps & 0x7) >> 2;
				_print_small ("OEM SETTINGS",    -1,  10);
                if (_dash.status & XCPU_STATE_MILES)
					_print_small ("Km/Mi: Miles", -1, _hei[0]);
				else
					_print_small ("Km/Mi: Km",    -1, _hei[0]);
				_print_small ("Wheel const.",    -1, _hei[1]);
				_print_small ("Throttle adjust", -1, _hei[2]);				
				_print_small ("Lights off",      -1, _hei[3]);
				_print_small ("Sensor monitor",  -1, _hei[4]);
                if ( _frame_dumps & 0x8)
					_print_small ( ">>", 20, _hei[_fact_menu_mode]-1);

                if ( event_happening ( menu_move,1))
					_fact_menu_mode++, _fact_menu_mode %= 5;
				if ( event_happening ( menu_press,1))
				{
					switch ( _fact_menu_mode)
					{
						case 0:  _switch_units_cnt = RESEND_TIMES; break;
						case 1: *status = ST_ADJUST_WHEEL_DIA;     break;
						case 2: *status = ST_ADJUST_THROTTLE_MAX;  break;
						case 3: _lights_off_cnt = RESEND_TIMES;
						        *status = ST_DASH;
						        break;
						case 4: *status = ST_DEBUG_INPUT; break;
						default: assert(0);
					}
				}
                if ( _input_status & HORN_MASK)
					*status = ST_DASH;
			}
			break;

		case ST_ADJUST_DRIVE_MODE:
			{
				const char _hei [] = { 30, 45, 60};
				const EVREC_CHECK speed_adj_exit[]= {{CRUISE_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				const EVREC_CHECK mode_adj[]= {{BRAKE_RIGHT_MASK, CHECK_RELEASE},{0x00000000,CHECK_END}};
				int anm = (_frame_dumps & 0x7) >> 2;
				_print_small ("DRIVE MODES", -1, 14);
				_print_small ("Soft",        -1, _hei[0]);
				_print_small ("Eco",         -1, _hei[1]);
				_print_small ("Racing",      -1, _hei[2]);
                if ( _frame_dumps & 0x8)
					_print_small ( ">>", 24, _hei[_adj_drive_mode]);
                if ( event_happening ( mode_adj,1))
					_adj_drive_mode++, _adj_drive_mode %= 3;
				if ( event_happening ( speed_adj_exit,8))
					_adj_drive_mode++, _adj_drive_mode_cnt = RESEND_TIMES, *status = ST_DASH;
			}
			break;
	}
}


void main()
{
	lcd_initialize();

	exos_port_create(&_can_rx_port, NULL);
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
    int initial_status = ST_LOGO_IN; //ST_LOGO_IN; //ST_DASH; 
	                                 //ST_DEBUG_INPUT; // ST_ADJUST_WHEEL_DIA; 
									 //ST_FACTORY_MENU
	int status =  initial_status;
	int prev_cpu_state = 0;	// Default state is OFF, wait for master to start
	while(1)
	{
		//_dash.status |= XCPU_STATE_ON;
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
			unsigned int req_update_time = MAIN_LOOP_TIME;
			time -= time_base;
			unsigned int elapsed_time = time - prev_time;

			_read_send_analogic_inputs ( status);

			_clean_bilevel ( &_screen);


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
				lcd_dump_screen ( _screen.pixels);
                screen_count=0;
				_frame_dumps++;
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
				_clean_bilevel ( &_screen);
				lcd_dump_screen ( _screen.pixels);
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
		case 0x301:
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

