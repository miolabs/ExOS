#include "relay.h"
#include <support/lpc17/can.h>

void relay_initialize()
{
	hal_can_initialize(0, 250000);
	hal_can_initialize(1, 250000);
}

void relay_set(int unit, unsigned long mask, unsigned long time)
{
	CAN_BUFFER data = (CAN_BUFFER) { .u32[0] = mask, .u32[1] = time };
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = unit }, &data, 8, CANF_PRI_ANY);
}
