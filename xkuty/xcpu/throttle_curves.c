
#include <assert.h>

#include "throttle_curves.h"

#define LEN_CURVES  (8+1)

// Fixed point 15
#define CFX(V)  ((unsigned short)(V * 32768.0f))
/*
static const unsigned short _curve_soft [ LEN_CURVES] =
{
	CFX(0.0),
	CFX(0.02),
	CFX(0.04),
	CFX(0.06),
	CFX(0.08),
	CFX(0.1),
	CFX(0.2),
	CFX(0.6),
	CFX(1.0)
};

static const unsigned short _curve_eco [ LEN_CURVES] =
{
	CFX(0.0),
	CFX(0.1),
	CFX(0.2),
	CFX(0.3),
	CFX(0.4),
	CFX(0.5),
	CFX(0.6),
	CFX(0.7),
	CFX(0.8)
};

static const unsigned short _curve_racing [ LEN_CURVES] =
{
	CFX(0.0),
	CFX(0.3),
	CFX(0.6),
	CFX(0.8),
	CFX(0.85),
	CFX(0.9),
	CFX(0.95),
	CFX(1.0),

	CFX(1.0)
};
*/

static const unsigned short _curve_soft [ LEN_CURVES] =
{
	CFX(0.2),
	CFX(0.3),
	CFX(0.3),
	CFX(0.4),
	CFX(0.5),
	CFX(0.6),
	CFX(0.7),
	CFX(0.8),

	CFX(1.0)
};

static const unsigned short _curve_eco [ LEN_CURVES] =
{
	CFX(0.2),
	CFX(0.3),
	CFX(0.3),
	CFX(0.4),
	CFX(0.5),
	CFX(0.6),
	CFX(0.7),
	CFX(0.8),

	CFX(0.8)
};

static const unsigned short _curve_racing [ LEN_CURVES] =
{
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),
	CFX(1.0),

	CFX(1.0)
};


static const unsigned short* _curves [] = 
{
	_curve_soft, // DEFAULT
	_curve_soft,
	_curve_eco,
	_curve_racing,
};


int get_curve_value ( int fx12_in, CURVE_MODE mode)
{
	fx12_in = __LIMIT( fx12_in, 0, 0x1000);
	int idx = fx12_in >> 9;		// 3 higher bits for array index
	int fac = fx12_in & 0x1ff;  // 9 lower bits as linear interpolation
	const unsigned short* c = _curves[mode];
	int res = ( c[idx] * (0x200 - fac) + c[idx+1] * fac) >> (15+9-12);
	if ( res > 0xfff) res = 0xfff;	// 1.0 not supported!
	return res;
}