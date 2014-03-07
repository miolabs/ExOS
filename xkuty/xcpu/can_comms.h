#ifndef XCPU_CAN_COMMS_H
#define XCPU_CAN_COMMS_H

#include <xkuty/xcpu.h>

typedef struct
{
	unsigned char Speed;
	unsigned char BattLevel;
	unsigned char State;
	unsigned char DriveMode;
	unsigned long Distance;
} XCPU_CAN_REPORT;

typedef struct
{
	unsigned char ThrottleMax;
	unsigned char ThrottleMin;
	unsigned char WheelRatio;
	unsigned char MaxSpeed;
} XCPU_CAN_REPORT_ADJUST;

void xcpu_can_initialize();
void xcpu_can_send_messages(XCPU_CAN_REPORT *report, XCPU_CAN_REPORT_ADJUST *adj);
XCPU_MSG *xcpu_can_get_message();
void xcpu_can_release_message(XCPU_MSG *xmsg);

void xcpu_received_master_input(XCPU_MASTER_INPUT *);


#endif // XCPU_CAN_COMMS_H
