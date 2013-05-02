#ifndef XCPU_H
#define XCPU_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef struct 
{
	unsigned char speed;			// kmh (or mph)
	unsigned char battery_level_fx8; 
	unsigned char status;			// XCPU_STATE;
	unsigned char speed_adjust;		// -10 to +10
	unsigned long distance;			// distance km (or miles)

} XCPU_MASTER_OUT;

typedef struct
{
	EXOS_MESSAGE;
	CAN_MSG  CanMsg;
} XCPU_MSG;

typedef enum
{
	XCPU_STATE_OFF = 0,
	XCPU_STATE_ON = (1<<0),
	XCPU_STATE_NEUTRAL = (1<<1),
	XCPU_STATE_CRUISE_ON = (1<<2),
	XCPU_STATE_WARNING = (1<<3),
	XCPU_STATE_ERROR = (1<<4),
	XCPU_STATE_MILES = (1<<5)	// Bit 0=KM, 1=MILES
} XCPU_STATE;

typedef enum
{
	XCPU_RELAY_CRUISIN = (1<<0),
	XCPU_RELAY_HORN = (1<<1),
	XCPU_RELAY_ADJUST_UP = (1<<2),
	XCPU_RELAY_ADJUST_DOWN = (1<<3),
	XCPU_RELAY_CHANGE_METRICS = (1<<4),
} XCPU_RELAYS_IN;

#endif // XCPU_H


