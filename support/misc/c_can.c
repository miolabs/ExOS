#include "c_can.h"
#include <kernel/mutex.h>

static unsigned long _usage[32];
static int _fullcan_reserved = 0;
static EXOS_MUTEX _lock;
#ifdef DEBUG
static unsigned long _errors_ack = 0;
static unsigned long _errors_other = 0;
static unsigned long _resets_tx = 0;
static unsigned long _unhandled_irq= 0;
#endif

int ccan_initialize(C_CAN_MODULE *can, unsigned int pclk, unsigned int bitrate, CAN_INIT_FLAGS initf)
{
	for(int i = 0; i < 32; i++) _usage[i] = 0;
	_fullcan_reserved = 0;
	exos_mutex_create(&_lock);

//	LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CAN;
//	LPC_SYSCON->PRESETCTRL |= PRESETCTRL_CAN_RST_N;
	can->CLKDIV = 0;

	can->CNTL = C_CAN_CNTL_INIT | C_CAN_CNTL_CCE;

	// obtain a 12MHz CAN clock
	unsigned long bit_clock = bitrate * 12;  
	int brp = pclk / bit_clock;
	if ((pclk % bit_clock) != 0) return 0;	 // clock must be exact
	can->BT = C_CANBT_F((brp - 1), 1, 8, 1);  // BRP, SJW, TSEG1, TSEG2		
	can->BRPE = 0;
		
	C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1
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

	can->CNTL = C_CAN_CNTL_IE | C_CAN_CNTL_EIE | C_CAN_CNTL_SIE
		| (initf & CAN_INITF_DISABLE_RETRANSMISSION ? C_CAN_CNTL_DAR : 0)	// disable automatic retransmission
		;

//	NVIC_EnableIRQ(CAN_IRQn);
//	NVIC_SetPriority(CAN_IRQn, 3);
	return 1;
}

static int _get_free_index(int *pindex, unsigned long id)
{
	int done = 0;
	exos_mutex_lock(&_lock);

	for(int i = _fullcan_reserved; i < 32; i++)
	{
		if (_usage[i] == 0 ||
			_usage[i] == id)
		{
			*pindex = i;
			_usage[i] = id;
			done = 1;
			break;
		}
	}
	exos_mutex_unlock(&_lock);
	return done;
}

void ccan_cancel_tx(C_CAN_MODULE *can)
{
#ifdef DEBUG
	_resets_tx++;
#endif
	C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1

	exos_mutex_lock(&_lock);

	unsigned long stall =  can->ND1 | (can->ND2 << 16);

	for(int i = 32 - 1; i >= _fullcan_reserved; i--)
	{
		if ((stall & (1 << i)) && _usage[i] != 0)
		{
			fn->CMDMSK = C_CAN_CMDMSK_ARB
				| C_CAN_CMDMSK_NEWDAT | C_CAN_CMDMSK_CLRINTPND
				| C_CAN_CMDMSK_CTRL;
			fn->CMDREQ = i + 1;

			fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
				| C_CAN_CMDMSK_CTRL | C_CAN_CMDMSK_WR;
			fn->ARB1 = fn->ARB2 = 0; // clear id and MSGVAL
			fn->MCTRL = 0; // clear DCL and request bits
			fn->CMDREQ = i + 1; // make transfer

			_usage[i] = 0;
		}
	}

	exos_mutex_unlock(&_lock);
}

void ccan_isr(C_CAN_MODULE *can)
{
	while(1)
	{
		unsigned short ir = can->INT;

		if (ir & 0x8000)
		{
			unsigned char stat = can->STAT;
			unsigned long txreq;

			switch(stat & 0x7)
			{
				case C_CAN_ERROR_ACK:
#ifdef DEBUG
					_errors_ack++;
#endif
					// FIXME: dirty hack for receiver not present / not working / not listening
					txreq = can->TXREQ1 | (can->TXREQ2 << 16); 
					for(int i = 0; i < 32; i++)
					{
						if (txreq & (1<<i))
						{
							C_CAN_FUNCTION *fn = &can->Interface2;	// use function 2
							fn->CMDMSK = C_CAN_CMDMSK_ARB | C_CAN_CMDMSK_CLRINTPND | C_CAN_CMDMSK_WR;
							fn->ARB1 = fn->ARB2 = 0;
							fn->CMDREQ = i + 1;
							_usage[i] = 0;
							break;
						}
					}
					break;
#ifdef DEBUG
				case C_CAN_ERROR_NONE:
					break;
				
				default:
					_errors_other++;
					break;
#endif 
			}
		}
		else
		{
			int intid = ir & 0x3f;
			if (intid == 0) break;
		
			C_CAN_FUNCTION *fn = &can->Interface2;	// use function 2
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
			else if (fn->MCTRL & C_CAN_MCTRL_INTPND)
			{
				// message successfully transmitted
				_usage[intid - 1] = 0;
			}
#ifdef DEBUG
			else _unhandled_irq++;
#endif
		}
	}
//	NVIC_ClearPendingIRQ(CAN_IRQn);
}

int ccan_send(C_CAN_MODULE *can, unsigned int id, C_CAN_DATA *data, CAN_MSG_FLAGS flags)
{
	int index;
	int done = _get_free_index(&index, id);
	if (!done)
	{
		hal_can_cancel_tx();
		done = _get_free_index(&index, id);
	}
	if (done)
	{
		CAN_BUFFER *buf = data->Buffer;
		C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1

		fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
			| C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B
			| C_CAN_CMDMSK_CTRL | C_CAN_CMDMSK_TXRQST
			| C_CAN_CMDMSK_WR;
		int dlc = data->Length & 0xf;
		if (dlc >= 1) fn->DA1 = buf->u8[0] | (buf->u8[1] << 8);
		if (dlc >= 3) fn->DA2 = buf->u8[2] | (buf->u8[3] << 8);
		if (dlc >= 5) fn->DB1 = buf->u8[4] | (buf->u8[5] << 8);
		if (dlc >= 7) fn->DB2 = buf->u8[6] | (buf->u8[7] << 8);
	
		if (flags & CANF_EXTID)
		{
			id &= 0x1fffffff;
			fn->ARB1 = id;
			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR | C_CAN_ARB2_XTD;
		}
		else
		{
			id = (id & 0x7ff) << 18;
			fn->ARB1 = id;
			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR;
		}
		fn->MCTRL = (data->Length & C_CAN_MCTRL_DLC_MASK) 
			| C_CAN_MCTRL_EOB
			| C_CAN_MCTRL_TXIE
			| C_CAN_MCTRL_NEWDAT;
		
		// make transfer
		fn->CMDREQ = index + 1;
	}
	return done;
}

int ccan_setup(C_CAN_MODULE *can, CAN_SETUP_CALLBACK callback, void *state)
{
	int count = 0;
    exos_mutex_lock(&_lock);

	while(count < C_CAN_MESSAGES)
	{
		CAN_EP ep;
		CAN_MSG_FLAGS flags = CANF_NONE;
        CAN_SETUP_CODE code = callback(count, &ep, &flags, state);
		if (code == CAN_SETUP_END)
			break;
		
		_usage[count] = 2;

		C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1
		fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
			| C_CAN_CMDMSK_CTRL //| C_CAN_CMDMSK_TXRQST
			| C_CAN_CMDMSK_WR;
	
		if (flags & CANF_EXTID)
		{
			int id = (ep.Id & 0x1fffffff);
			fn->ARB1 = id;
			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_XTD;
		}
		else
		{
			int id = (ep.Id & 0x7ff) << 18;
			fn->ARB1 = id;
			fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL;
		}
		
		switch(code)
		{
			case CAN_SETUP_RX:
				fn->MCTRL = (8 & C_CAN_MCTRL_DLC_MASK) 
					| ((flags & CANF_RXINT) ? C_CAN_MCTRL_RXIE : 0);
				break;
			case CAN_SETUP_TX:
				fn->ARB2 |= C_CAN_ARB2_DIR;
				fn->MCTRL = (8 & C_CAN_MCTRL_DLC_MASK)
					| C_CAN_MCTRL_EOB;
//					| C_CAN_MCTRL_TXIE
//					| C_CAN_MCTRL_NEWDAT;
				break;
		}
		fn->CMDREQ = ++count; // make transfer
	}
	_fullcan_reserved = count;

	exos_mutex_unlock(&_lock);
	return count;
}

void ccan_write_msg(C_CAN_MODULE *can, int index, CAN_MSG *msg)
{
	C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1

	fn->CMDMSK = C_CAN_CMDMSK_ARB // transfer id, DIR, XTD, MSGVAL
		| C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B
		| C_CAN_CMDMSK_CTRL | C_CAN_CMDMSK_TXRQST
		| C_CAN_CMDMSK_WR;
	CAN_BUFFER *buf = &msg->Data;
	int dlc = msg->Length & 0xf;
	if (dlc >= 1) fn->DA1 = buf->u8[0] | (buf->u8[1] << 8);
	if (dlc >= 3) fn->DA2 = buf->u8[2] | (buf->u8[3] << 8);
	if (dlc >= 5) fn->DB1 = buf->u8[4] | (buf->u8[5] << 8);
	if (dlc >= 7) fn->DB2 = buf->u8[6] | (buf->u8[7] << 8);

	if (msg->Flags & CANF_EXTID)
	{
		int id = msg->EP.Id & 0x1fffffff;
		fn->ARB1 = id;
		fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR | C_CAN_ARB2_XTD;
	}
	else
	{
		int id = (msg->EP.Id & 0x7ff) << 18;
		fn->ARB1 = id;
		fn->ARB2 = (id >> 16) | C_CAN_ARB2_MSGVAL | C_CAN_ARB2_DIR;
	}
	fn->MCTRL = (msg->Length & C_CAN_MCTRL_DLC_MASK) 
		| C_CAN_MCTRL_EOB
		| C_CAN_MCTRL_TXIE
		| C_CAN_MCTRL_NEWDAT;
	
	// make transfer
	fn->CMDREQ = index + 1;
}

void ccan_write_data(C_CAN_MODULE *can, int index, CAN_BUFFER *data, unsigned int length)
{
	C_CAN_FUNCTION *fn = &can->Interface1;	// use function 1

	int dlc = length & 0xf;
	if (dlc >= 1) fn->DA1 = data->u8[0] | (data->u8[1] << 8);
	if (dlc >= 3) fn->DA2 = data->u8[2] | (data->u8[3] << 8);
	if (dlc >= 5) fn->DB1 = data->u8[4] | (data->u8[5] << 8);
	if (dlc >= 7) fn->DB2 = data->u8[6] | (data->u8[7] << 8);

	fn->MCTRL = (length & C_CAN_MCTRL_DLC_MASK) 
		| C_CAN_MCTRL_EOB
		| C_CAN_MCTRL_TXIE
		| C_CAN_MCTRL_NEWDAT;

	fn->CMDMSK = C_CAN_CMDMSK_DATA_A | C_CAN_CMDMSK_DATA_B |
		C_CAN_CMDMSK_CTRL | C_CAN_CMDMSK_TXRQST |
		C_CAN_CMDMSK_WR;

	fn->CMDREQ = index + 1;
}

