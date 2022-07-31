#include "cpu.h"
#include <support/misc/c_can.h>

static C_CAN_MODULE *const _can = (C_CAN_MODULE*)LPC_CAN;

int hal_can_initialize(int module, int bitrate, CAN_INIT_FLAGS initf)
{
	if (module == 0)
	{
		LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CAN;
		LPC_SYSCON->PRESETCTRL |= PRESETCTRL_CAN_RST_N;
	
		if (ccan_initialize(_can, SystemCoreClock, bitrate, initf))
		{
			NVIC_EnableIRQ(CAN_IRQn);
			NVIC_SetPriority(CAN_IRQn, 3);
			return 1;
		}
	}
	return 0;
}

void hal_can_cancel_tx()
{
	ccan_cancel_tx(_can);
}

void CAN_IRQHandler()
{
	ccan_isr(_can);
	NVIC_ClearPendingIRQ(CAN_IRQn);
}

int hal_can_send(CAN_EP ep, CAN_BUFFER *data, unsigned char length, CAN_MSG_FLAGS flags)
{
	C_CAN_DATA ccan_data = (C_CAN_DATA) { .Buffer = data, .Length = length };
	return ccan_send(_can, ep.Id, &ccan_data, flags);
}

int hal_can_setup(CAN_SETUP_CALLBACK callback, void *state)
{
	return ccan_setup(_can, callback, state);
}

int hal_fullcan_write_msg(int index, CAN_MSG *msg)
{
	ccan_write_msg(_can, index, msg);
	return 1;
}

int hal_can_write_data(int index, CAN_BUFFER *data, unsigned int length)
{
	ccan_write_data(_can, index, data, length);
	return 1;
}




