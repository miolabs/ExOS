

typedef enum
{
	CURVE_SOFT   = 1,
	CURVE_ECO    = 2,
	CURVE_RACING = 3,
} CURVE_MODE;

// Return a fixed12 value
int get_curve_value ( int fx12_in, CURVE_MODE mode);