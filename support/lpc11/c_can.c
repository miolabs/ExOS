#include "cpu.h"
#include "c_can.h"
#include <support/can_hal.h>

static C_CAN_MODULE *const _can = (C_CAN_MODULE*)LPC_CAN;
static int _send_index = 0;

int hal_can_initialize(int module, int bitrate)
{
	if (module != 0)
		return 0;

	LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CAN;
	LPC_SYSCON->PRESETCTRL |= PRESETCTRL_CAN_RST_N;
	_can->CLKDIV = 0;

	_can->CNTL = C_CAN_CNTL_INIT | C_CAN_CNTL_CCE;

	// obtain a 12MHz CAN clock
	unsigned long bit_clock = bitrate * 12;  
	int brp = SystemCoreClock / bit_clock;
	if ((SystemCoreClock % 12000000) != 0) return 0;	 // clock must be exact
	_can->BT = C_CANBT_F((brp - 1), 1, 8, 1);  // BRP, SJW, TSEG1, TSEG2		
	_can->BRPE = 0;
		
	C_CAN_FUNCTION *fn = &_can->Interface1;	// use function 1
	for (int i = 0; i < 32; i++)
	{
#ifdef DEBUG
		// read contents on object
		fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
			| C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B;
		fn->CMDREQ = i + 1;
#endif
		fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
			| C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B
			| C_CAN_CMDMSK_MASK | C_CAN_CMDMSK_CTRL
			| C_CAN_CMDMSK_WR;
		fn->DA1 = fn->DA2 = fn->DB1 = fn->DB2 = i + 1 | (i << 8); 
		fn->ARB1 = fn->ARB2 = 0; // clear id and MSGVAL
		fn->MASK1 = fn->MASK2 = 0;
		fn->MCTRL = 0; // clear DCL and request bits
		// make transfer
		fn->CMDREQ = i + 1;
	}

	_can->CNTL = C_CAN_CNTL_IE //| C_CAN_CNTL_SIE | C_CAN_CNTL_EIE
		| C_CAN_CNTL_DAR	// disable automatic retransmission
		;

	NVIC_EnableIRQ(CAN_IRQn);
	NVIC_SetPriority(CAN_IRQn, 3);
	return 1;
}

void CAN_IRQHandler()
{
	while(1)
	{
		unsigned short ir = _can->INT;
		int intid = ir & 0x3f;
		if (intid == 0) break;
	
		C_CAN_FUNCTION *fn = &_can->Interface2;	// use function 2
		fn->CMDMSK = C_CAN_CMDMSK_ARB | C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B 
			| C_CAN_CMDMSK_NEWDAT | C_CAN_CMDMSK_CLRINTPND
			| C_CAN_CMDMSK_CTRL;
		fn->CMDREQ = intid;
		
		if (fn->MCTRL & C_CAN_MCTRL_NEWDAT)
		{
			CAN_MSG msg;
			if (fn->ARB2 & C_CAN_ARB2_XTD)
			{
				msg.EP = (CAN_EP) { .Id = ((fn->ARB2 << 16) & 0x1FFF) | fn->ARB1 };
				msg.Flags = CANF_EXTID;
			}
			else
			{
				msg.EP = (CAN_EP) { .Id = (fn->ARB2 >> 2) & 0x7FF };
				msg.Flags = 0;
			}
			msg.Length = fn->MCTRL & C_CAN_MCTRL_DLC_MASK;
			msg.Data.u16[0] = fn->DA1;
			msg.Data.u16[1] = fn->DA2;
			msg.Data.u16[2] = fn->DB1;
			msg.Data.u16[3] = fn->DB2;
            hal_can_received_handler(intid - 1, &msg);
		}
	}
	NVIC_ClearPendingIRQ(CAN_IRQn);
}

int hal_can_send(CAN_EP ep, CAN_BUFFER *data, unsigned char length, CAN_MSG_FLAGS flags)
{
	C_CAN_FUNCTION *fn = &_can->Interface1;	// use function 1
	fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
		| C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B
		| C_CAN_CMDMSK_CTRL | C_CAN_CMDMSK_TXRQST
		| C_CAN_CMDMSK_WR;
	int dlc = length & 0xf;
	if (dlc >= 1) fn->DA1 = data->u8[0] | (data->u8[1] << 8);
	if (dlc >= 3) fn->DA2 = data->u8[2] | (data->u8[3] << 8);
	if (dlc >= 5) fn->DB1 = data->u8[4] | (data->u8[5] << 8);
	if (dlc >= 7) fn->DB2 = data->u8[6] | (data->u8[7] << 8);

	if (flags & CANF_EXTID)
	{
		int id = ep.Id & 0x1fffffff;
		fn->ARB1 = id;
		fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR | C_CAN_ARB2_XTD;
	}
	else
	{
		int id = (ep.Id & 0x7ff) << 18;
		fn->ARB1 = id;
		fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR;
	}
	fn->MCTRL = (length & C_CAN_MCTRL_DLC_MASK) 
		| C_CAN_MCTRL_EOB
//		| C_CAN_MCTRL_TXIE
		| C_CAN_MCTRL_NEWDAT;
	
	// make transfer
	int index = _send_index & 0x1F;
	fn->CMDREQ = index + 1;

	unsigned long reqmask, objmask = (1 << index);
	do 
	{ 
		reqmask = _can->TXREQ1 | (_can->TXREQ2 << 16);
	} while (objmask & reqmask);
}

int hal_fullcan_setup(HAL_FULLCAN_SETUP_CALLBACK callback, void *state)
{
	int count = 0;
	while(count < C_CAN_MESSAGES)
	{
		CAN_EP ep;
		CAN_MSG_FLAGS flags = CANF_NONE;
		if (!callback(count, &ep, &flags, state))
			break;
		
		count++;

		C_CAN_FUNCTION *fn = &_can->Interface1;	// use function 1
		fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
			| C_CAN_CMDMSK_CTRL //| C_CAN_CMDMSK_TXRQST
			| C_CAN_CMDMSK_WR;
//		int dlc = obj->dlc & 0xf;
//		if (dlc >= 1) fn->DA1 = obj->data[0] | (obj->data[1] << 8);
//		if (dlc >= 3) fn->DA2 = obj->data[2] | (obj->data[3] << 8);
//		if (dlc >= 5) fn->DB1 = obj->data[4] | (obj->data[5] << 8);
//		if (dlc >= 7) fn->DB2 = obj->data[6] | (obj->data[7] << 8);
	
//		if (mode_id & CAN_MSGOBJ_EXT)
//		{
//			int id = mode_id & 0x1fffffff;
//			fn->ARB1 = id;
//			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_XTD;
//		}
//		else
//		{
			int id = (ep.Id & 0x7ff) << 18;
			fn->ARB1 = id;
			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL;
//		}
		fn->MCTRL = (8 & C_CAN_MCTRL_DLC_MASK) 
			| ((flags & CANF_RXINT) ? C_CAN_MCTRL_EOB : 0)
			| C_CAN_MCTRL_RXIE;
	
		fn->CMDREQ = count; // make transfer
	}
	_send_index = count;
	return count;
}


