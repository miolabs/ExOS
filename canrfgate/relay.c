#include "relay.h"
#include <support/lpc17/can.h>

void relay_initialize()
{
	hal_can_initialize(0, 250000, CAN_INITF_DISABLE_RETRANSMISSION);
	hal_can_initialize(1, 250000, CAN_INITF_DISABLE_RETRANSMISSION);
}

void relay_set(int unit, unsigned short mask, unsigned short value, unsigned long time)
{
	CAN_BUFFER data;
	data.u16[0] = mask;
	data.u16[1] = value;
	data.u32[1] = time;
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = unit }, &data, 8, CANF_PRI_ANY);
}
