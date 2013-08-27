#ifndef XCPU_H
#define XCPU_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef enum
{
	XCPU_DRIVE_MODE_SOFT = 0,
	XCPU_DRIVE_MODE_ECO,
	XCPU_DRIVE_MODE_RACING,
	XCPU_DRIVE_MODE_CUSTOM
} XCPU_DRIVE_MODE;

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
	unsigned short drive_mode;			// XCPU_DRIVE_MODE
	unsigned long  reserved2;	
} XCPU_MASTER_OUT2;

typedef struct
{
	EXOS_MESSAGE;
	CAN_MSG CanMsg;
} XCPU_MSG;

typedef enum
{
	XCPU_STATE_OFF = 0,
	XCPU_STATE_ON = (1<<0),
	XCPU_STATE_NEUTRAL = (1<<1),
	XCPU_STATE_CRUISE_ON = (1<<2),
	XCPU_STATE_WARNING = (1<<3),
	XCPU_STATE_ERROR = (1<<4),
	XCPU_STATE_MILES = (1<<5),	// Bit 0=KM, 1=MILES
	XCPU_STATE_LIGHT_OFF =(1<<6),
} XCPU_STATE;


typedef enum
{
	XCPU_BUTTON_CRUISE = (1<<0),
	XCPU_BUTTON_HORN = (1<<1),
	XCPU_BUTTON_BRAKE_REAR = (1<<2),
	XCPU_BUTTON_BRAKE_FRONT = (1<<3),
} XCPU_BUTTONS;

typedef enum
{
	XCPU_EVENT_ADJUST_UP = (1<<0),
	XCPU_EVENT_ADJUST_DOWN = (1<<1),
	XCPU_EVENT_SWITCH_UNITS = (1<<2),
	XCPU_EVENT_ADJUST_THROTTLE = (1<<3),
	XCPU_EVENT_ADJUST_DRIVE_MODE = (3<<4),	// 1-3
	XCPU_EVENT_SWITCH_LIGHTS = (1<<6),
	XCPU_EVENT_CONFIGURING = (1<<7), 
} XCPU_EVENTS;

typedef enum
{
	XCPU_IOS_EVENT_POWER_ON = (1<<0),
	XCPU_IOS_EVENT_POWER_OFF = (1<<1),
	XCPU_IOS_EVENT_ADJUST_DRIVE_MODE= (1<<2),
	XCPU_IOS_EVENT_SET_CURVE = (1<<3),
} XCPU_IPHONE_EVENTS;

#define XCPU_EVENT_ADJUST_DRIVE_MODE_SHIFT (4)



#endif // XCPU_H


