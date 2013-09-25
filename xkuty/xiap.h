#ifndef XKUTY_IAP_H
#define XKUTY_IAP_H

#include "xdisplay.h"

#define XIAP_MAGIC ('X' | ('K' << 8) | ('U' << 16) | ('1' << 24)) 

typedef struct __attribute__((__packed__))
{
	unsigned long Magic;
	unsigned short Speed;
	unsigned short StatusFlags;
	unsigned long Distance;
	unsigned char Battery;
    unsigned char DriveMode;
//	unsigned char CustomCurve[7];
} XIAP_FRAME_TO_IOS;

void xiap_send_frame(DASH_DATA *dash);

typedef struct __attribute__((__packed__))
{
	unsigned long Magic;
	unsigned char Command;
	unsigned char Data[8];
} XIAP_FRAME_FROM_IOS;

typedef enum
{
	IOS_COMMAND_NONE = 0,
	IOS_COMMAND_POWER_OFF = 1,
	IOS_COMMAND_POWER_ON = 2,
	IOS_COMMAND_ADJUST_DRIVE_MODE = 3,
    IOS_COMMAND_SET_CUSTOM_CURVE = 4
} IOS_COMMANDS;

int xiap_get_frame(XIAP_FRAME_FROM_IOS *frame);

#endif // XKUTY_IAP_H