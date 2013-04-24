#ifndef XCPU_H
#define XCPU_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef struct
{
	EXOS_MESSAGE;
	CAN_MSG CanMsg;
} XCPU_MSG;

#endif // XCPU_H


