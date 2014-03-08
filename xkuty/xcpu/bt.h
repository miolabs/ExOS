#ifndef XCPU_BT_H
#define XCPU_BT_H

#include <xkuty/xcpu.h>

typedef struct
{
	unsigned char Speed;
	unsigned char BattLevel;
	unsigned char State;
	unsigned char DriveMode;
	unsigned long Distance;
} XCPU_BT_STATE_CHAR_DATA;

typedef enum
{
	XCPU_BT_DISCONNECTED = 0,
	XCPU_BT_CONNECTING,
	XCPU_BT_CONNECTED,
} XCPU_BT_STATE;

int xcpu_bt_initialize();
int xcpu_bt_update(XCPU_BT_STATE_CHAR_DATA *data);

#endif // XCPU_BT_H

