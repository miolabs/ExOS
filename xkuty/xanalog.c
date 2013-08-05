#include "xanalog.h"
#include <support/adc_hal.h>

static ANALOG_INPUT _ain[ANALOG_INPUT_COUNT];

void xanalog_initialize()
{
	hal_adc_initialize(1000, 16);

	_ain[THROTTLE_IDX] = (ANALOG_INPUT) { .Min = 0, .Max = 0xffff, .DefMin = 587, .DefMax = 3109};	// 2400->maximum accepted by motor controller, 1050->minimum 
	_ain[BRAKE_REAR_IDX] = (ANALOG_INPUT) { .Min = 0, .Max = 0xffff, .DefMin = 1810, .DefMax = 2539};
	_ain[BRAKE_FRONT_IDX] = (ANALOG_INPUT) { .Min = 0, .Max = 0xffff, .DefMin = 1810, .DefMax = 2539};
	_ain[CRUISE_IDX] = (ANALOG_INPUT) { .Min = 0, .Max = 0xffff, .DefMin = 0, .DefMax = 4095};
	_ain[HORN_IDX] = (ANALOG_INPUT) { .Min = 0, .Max = 0xffff, .DefMin = 0, .DefMax = 4095};

	xanalog_reset_filters();
}

void xanalog_reset_filters()
{
#ifdef GLITCHY_AD
	fir_init( &_ain[THROTTLE_IDX].Fir,    6,  1, 500, 1);
	fir_init( &_ain[BRAKE_REAR_IDX].Fir,  10, 1, 100, 3);
	fir_init( &_ain[BRAKE_FRONT_IDX].Fir, 10, 1, 100, 5);
	fir_init( &_ain[CRUISE_IDX].Fir,      8, 0, 0, 0);
	fir_init( &_ain[HORN_IDX].Fir,        8, 0, 0, 0);
#else
	fir_init( &_ain[THROTTLE_IDX].Fir,    6, 1, 500, 2);
	fir_init( &_ain[BRAKE_REAR_IDX].Fir,  6, 1, 100, 2);
	fir_init( &_ain[BRAKE_FRONT_IDX].Fir, 6, 1, 100, 2);
	fir_init( &_ain[CRUISE_IDX].Fir,      6, 0, 0, 0);
	fir_init( &_ain[HORN_IDX].Fir,        6, 0, 0, 0);
#endif
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

void xanalog_update()
{
	for(int i = 0; i < ANALOG_INPUT_COUNT; i++)
	{
		_ain[i].Current = hal_adc_read(i) >> 4;	// 12 bit resolution
        _ain[i].Filtered = fir_filter(&_ain[i].Fir, _ain[i].Current);
		_ain[i].Scaled = _sensor_scale(_ain[i].Filtered, _ain[i].DefMin, _ain[i].DefMax, 0xfff);
		int tocmp = _ain[i].Filtered;
		if (tocmp > _ain[i].Max) _ain[i].Max = tocmp;
		if (tocmp < _ain[i].Min) _ain[i].Min = tocmp;
	}
}

XCPU_BUTTONS xanalog_read_digital()
{
	XCPU_BUTTONS buttons = 0;
	
	if (_ain[CRUISE_IDX].Filtered < 0x800)
		buttons |= XCPU_BUTTON_CRUISE;
	if (_ain[BRAKE_REAR_IDX].Scaled > 400) // ~10%
		buttons |= XCPU_BUTTON_BRAKE_REAR;
	if (_ain[BRAKE_FRONT_IDX].Scaled > 400) // ~10%
		buttons |= XCPU_BUTTON_BRAKE_FRONT;
	if (_ain[HORN_IDX].Filtered < 0x800)
		buttons |= XCPU_BUTTON_HORN;

	if (_ain[THROTTLE_IDX].Scaled > 200) // ~5%
		buttons |= XCPU_BUTTON_THROTTLE_OPEN;

	return buttons;
}

ANALOG_INPUT *xanalog_input(int index)
{
	return index < ANALOG_INPUT_COUNT ? &_ain[index] : NULL;
}

