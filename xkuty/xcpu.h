#ifndef XCPU_H
#define XCPU_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef enum
{
	XCPU_DRIVE_MODE_SOFT = 0,
	XCPU_DRIVE_MODE_ECO,
	XCPU_DRIVE_MODE_RACING,
	XCPU_DRIVE_MODE_CUSTOM,
	XCPU_DRIVE_MODE_COUNT
} XCPU_DRIVE_MODE;

typedef struct 
{
	unsigned char Speed;		// kmh (or mph)
	unsigned char BattLevel;	// fx8
	unsigned char State;		// XCPU_STATE;
	unsigned char DriveMode;
	unsigned long Distance;		// distance km (or miles) / 10		
} XCPU_MASTER_OUT1;

typedef struct 
{
	unsigned char  ThrottleMax;	// fx8
	unsigned char  ThrottleMin;	// fx8
	unsigned char  WheelRatio;	// -10 to +10
	unsigned char  MaxSpeed;
	unsigned long  Reserved;
} XCPU_MASTER_OUT2;

typedef struct
{
	unsigned short ThrottleRaw;
	unsigned char BrakeRear;
	unsigned char BrakeFront;
	unsigned short Events;
	unsigned short Buttons;
} XCPU_MASTER_INPUT;

typedef struct __attribute__((__packed__))
{
	unsigned char Cmd;	// XCPU_COMMANDS
	union __attribute__((__packed__))
	{
		unsigned char Data[7];
	};
} XCPU_MASTER_INPUT2;

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
	XCPU_STATE_LIGHT_OFF = (1<<6),	// FIXME: revise usage of this
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
	XCPU_EVENT_TURN_ON = (1<<3),
	XCPU_EVENT_TURN_OFF = (1<<4),
	XCPU_EVENT_SWITCH_LIGHTS = (1<<5),
	XCPU_EVENT_CONFIGURING = (1<<6),
	XCPU_EVENT_ENTER_BOOTLOADER = (1<<7),
    XCPU_EVENT_ADJUST_MAX_SPEED_DOWN = (1<<8), 
    XCPU_EVENT_ADJUST_MAX_SPEED_UP = (1<<9),
    XCPU_EVENT_NEXT_DRIVE_MODE = (1<<10),
} XCPU_EVENTS;

typedef enum
{
	XCPU_CMD_NOP = 0,
	XCPU_CMD_POWER_ON,
	XCPU_CMD_POWER_OFF,
	XCPU_CMD_SET_DRIVE_MODE,
	XCPU_CMD_SET_CURVE,
	XCPU_CMD_INVOKE_BOOTLOADER,
	XCPU_CMD_ADJUST_THROTTLE,
    XCPU_CMD_REMOVE_PHONE,
	XCPU_CMD_CONFIRM_PHONE
} XCPU_COMMANDS;

typedef enum 
{
	XCPU_MULTI_NOP = 0,
	XCPU_MULTI_CUSTOM_CURVE,
	XCPU_MULTI_PHONE_LOG
} XCPU_MULTIPACKET_MSGS;

#endif // XCPU_H


