#include <assert.h>

#include "throttle_curves.h"

#define LEN_CURVES  (7+1)

// Fixed point 15
#define CFX(V)  ((unsigned short)(V * 32768.0f))

static const unsigned short _curve_soft [ LEN_CURVES] =
{
	CFX(0.299805),
	CFX(0.391357),
	CFX(0.474854),
	CFX(0.599854),
	CFX(0.733154),
	CFX(0.866455),
	CFX(0.999756),

	CFX(0.999756)
};

static const unsigned short _curve_eco [ LEN_CURVES] =
{
	CFX(0.299805),
	CFX(0.391357),
	CFX(0.474854),
	CFX(0.599854),
	CFX(0.733154),
	CFX(0.799805),
	CFX(0.799805),

	CFX(0.799805)
};

static const unsigned short _curve_racing [ LEN_CURVES] =
{
	CFX(1.000000),
	CFX(1.000000),
	CFX(1.000000),
	CFX(1.000000),
	CFX(1.000000),
	CFX(1.000000),
	CFX(1.000000),

	CFX(1.000000)
};

const unsigned char* _custom_curve_src = 0;
static unsigned char _curve_custom [ LEN_CURVES];

static const unsigned short* _curves [] = 
{
	_curve_soft,
	_curve_eco,
	_curve_racing,
    0,	// Custom
};

int get_curve_value(int fx12_in, XCPU_DRIVE_MODE mode)
{
	if (mode < 0) 
		mode = 0;
	if (mode >= (sizeof(_curves) / sizeof(unsigned short*)))
		mode = (sizeof(_curves) / sizeof(unsigned short*)) - 1;
	fx12_in = __LIMIT(fx12_in, 0, 0x1000);
	// Scale to 7/8 for 7 values table
	fx12_in = (fx12_in * CFX(7.0/8.0)) >> 15;

	const unsigned short* c = _curves[mode];
	// Custom mode
	if ( c == 0)
	{
		int i;
		for (i = 0; i < (LEN_CURVES - 1); i++)
			_curve_custom[i] = _custom_curve_src[i] << (15 - 8);
		// Terminator!
		_curve_custom[LEN_CURVES - 1] = _curve_custom[LEN_CURVES - 2];
		c = (const unsigned short*)_curve_custom;
	}

	// Linear interpolation
	int idx = fx12_in >> 9;		// 3 higher bits for array index
	int fac = fx12_in & 0x1ff;  // 9 lower bits as linear interpolation

	int res = (c[idx] * (0x200 - fac) + c[idx + 1] * fac) >> (15 + 9 - 12);
	if ( res > 0xfff) 
		res = 0xfff;	// 1.0 not supported

	return res;
}

void set_custom_curve_ptr(const unsigned char* curve)
{
	_custom_curve_src = curve;
}