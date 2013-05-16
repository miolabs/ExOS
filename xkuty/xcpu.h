#ifndef XCPU_H
#define XCPU_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef struct 
{
	unsigned char speed;			// kmh (or mph)
	unsigned char battery_level_fx8; 
	unsigned char status;			// XCPU_STATE;
	signed char	  speed_adjust;		// -10 to +10
	unsigned long distance;			// distance km (or miles) / 10

} XCPU_MASTER_OUT1;

typedef struct 
{
	unsigned char  throttle_adj_min;	// fx8
	unsigned char  throttle_adj_max;	// fx8
	unsigned short reserved1; 
	unsigned long  reserved2;	

} XCPU_MASTER_OUT2;

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
	XCPU_BUTTON_CRUISE = (1<<0),
	XCPU_BUTTON_HORN = (1<<1),
	XCPU_BUTTON_ADJUST_UP = (1<<2),
	XCPU_BUTTON_ADJUST_DOWN = (1<<3),
	XCPU_BUTTON_SWITCH_UNITS = (1<<4),
} XCPU_BUTTONS;

#endif // XCPU_H


