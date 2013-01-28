#ifndef CAN_DRIVER_LPC17_H
#define CAN_DRIVER_LPC17_H

#include <net/adapter.h>
#include <support/can_hal.h>

typedef struct
{
	EXOS_NODE Node;

} EXOS_CAN_BUFFER;

extern const NET_DRIVER __can_driver_lpc17;

#endif // CAN_DRIVER_LPC17_H

