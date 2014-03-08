#ifndef XANALOG_H
#define XANALOG_H

#include "xcpu.h"
#include "fir.h"

enum
{
	THROTTLE_IDX = 0,
	BRAKE_REAR_IDX,
	BRAKE_FRONT_IDX,
	HORN_IDX,
	CRUISE_IDX,
	ANALOG_INPUT_COUNT
};

#define THROTTLE_MASK    (1<<THROTTLE_IDX)
#define BRAKE_REAR_MASK  (1<<BRAKE_REAR_IDX)
#define BRAKE_FRONT_MASK (1<<BRAKE_FRONT_IDX)
#define CRUISE_MASK      (1<<CRUISE_IDX)
#define HORN_MASK        (1<<HORN_IDX)

typedef struct
{
	unsigned short Current;			// Analog inputs, 12 bits
	unsigned short Filtered;		// Filtered value of curr
    unsigned short Scaled;			// Input re-scaled to def_min/def_max
	unsigned short Min, Max;		// For scaling
	FIR Fir;
} ANALOG_INPUT;


void xanalog_initialize();
void xanalog_reset_filters();
void xanalog_update();
XCPU_BUTTONS xanalog_read_digital();
ANALOG_INPUT *xanalog_input(int index);

#endif // XANALOG_H
