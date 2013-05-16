#include "can.h"
#include "cpu.h"
#include <support/board_hal.h>
#include <kernel/panic.h>

// NOTE: nominal bit time = (1 + (tseg1 + 1) + (tseg2 + 1))
#define CANBTR_F(brp, sjw, tseg1, tseg2, sam) ((brp & 0x3F) | ((sjw & 0x03) << 14) | ((tseg1 & 0x0F) << 16) | ((tseg2 & 0x07) << 20) | ((sam & 0x01) << 23))

#define CAN_MODULE_COUNT 2
static CAN_MODULE *_can_table[] = {
	(CAN_MODULE *)LPC_CAN1_BASE, (CAN_MODULE *)LPC_CAN2_BASE};
static FCAN_MODULE *_fcan = (FCAN_MODULE *)LPC_CANAF_BASE;
static unsigned char *_af_ram = (unsigned char *)LPC_CANAF_RAM_BASE;

static unsigned long volatile _can_reset_cnt[CAN_MODULE_COUNT];
static unsigned long volatile _can_error_cnt[CAN_MODULE_COUNT];
static unsigned long volatile _can_modules_enabled = 0;
static void _can_interrupt(int can);

void CAN_IRQHandler(void)
{
	if (_can_modules_enabled & (1<<0)) _can_interrupt(0);
    if (_can_modules_enabled & (1<<1)) _can_interrupt(1);
	if (_fcan->FCANIE & 1)
	{
		CAN_MSG msg;
		unsigned long mask = _fcan->FCANIC0;
		if (mask != 0) 
		{
			for(int i = 0; i < 32; i++)
			{
				if ((mask & (1 << i)) &&
					hal_fullcan_read_msg(i, &msg))
					hal_can_received_handler(i, &msg);
			}
		}
		mask = _fcan->FCANIC1;
		if (mask != 0) 
		{
			for(int i = 0; i < 32; i++)
			{
				if ((mask & (1 << i)) &&
					hal_fullcan_read_msg(32 + i, &msg))
					hal_can_received_handler(32 + i, &msg);
			}
		}
	}
}

int hal_can_initialize(int module, int bitrate)
{
	if (!hal_board_init_pinmux(HAL_RESOURCE_CAN, module))
		return 0;

#if (__TARGET_PROCESSOR < 1770)
	// NOTE: PCLK_CAN1 and PCLK_CAN2 must have the same PCLK divide value when the CAN function is used
	PCLKSEL0bits.PCLK_CAN1 = 1;
	PCLKSEL0bits.PCLK_CAN2 = 1;
#endif

	unsigned long btr;
	int cdiv;
	if ((SystemCoreClock % (12 * bitrate)) == 0)
	{
		int brp = SystemCoreClock / (12 * bitrate);
		btr = CANBTR_F((brp - 1), 1, 8, 1, 0);  // BRP, SJW, TSEG1, TSEG2, SAM (12 clk/bit)
	}
	else if ((SystemCoreClock % (5 * bitrate)) == 0)
	{
		int brp = SystemCoreClock / (5 * bitrate);
		btr = CANBTR_F((brp - 1), 1, 2, 0, 0);  // BRP, SJW, TSEG1, TSEG2, SAM (5 clk /bit)
	}
	else return 0;

	switch(module)
	{
		case 0:
			LPC_SC->PCONP |= PCONP_PCAN1;
			break;
		case 1:
			LPC_SC->PCONP |= PCONP_PCAN2;
			break;
		default:
			return 0;
	}

	if (_can_modules_enabled == 0)
	{
         NVIC_EnableIRQ(CAN_IRQn);
	}

	_can_modules_enabled |= (1<<module);
	CAN_MODULE *can = _can_table[module];
	can->MOD = 1;	// go into Reset Mode
	can->IER = 0;	// disable Interrupts
	can->GSR = 0;	// clear error counters

	can->BTR = btr;

	can->EWL = 0x60; // Error Warning Limit

	can->IER = CAN_RI | CAN_EI | CAN_BEI | CAN_DOI | 
		CAN_TI1 | CAN_TI2 | CAN_TI3;  // Enable Interrupts
	can->MOD = 4; // Operating Mode Normal (Self Test Mode - No ACK required)

	_fcan->AFMR = AFMR_AccBP; // bypass Accept Filter
	return 1;
}

static void _can_interrupt(int module)
{
	int error = 0;

	CAN_MODULE *can = _can_table[module];
    _CANICRbits icr = can->ICRbits;

    if (icr.RI)
    {
		// frame Receive
		CAN_MSG msg;
		msg.Data.u32[0] = can->RDA;
		msg.Data.u32[1] = can->RDB;
		msg.EP = (CAN_EP) { .Id = can->RID, .Bus = module };
		msg.Length = can->RFSbits.DLC;
		msg.Flags = can->RFSbits.RTR;
		can->CMR = CAN_CMR_RRB; // Release Receive Buffer
		hal_can_received_handler(-1, &msg);
	}

	if (icr.DOI)
	{
		// Data Overrun
		can->CMR = CAN_CMR_CDO;	// Clear Data Overrun
		_CANGSRbits gsr = can->GSRbits;
		if (gsr.BS)
		{
			can_reset(module);
			error = 1;
		}
	}

	if (icr.EI && !error) 
	{
		// BS or ES changed
		_CANGSRbits gsr = can->GSRbits;
		if (gsr.BS) 
		{
			can_reset(module);
			error = 1;
		}
	}
	
	if (icr.BEI && !error)
	{
		// Bus Error occured
		can_reset(module);
		error = 2;
	}

	if (error)
	{
#ifdef DEBUG
//		CAN_BUFFER data;
//		data.u32[0] =  _can_reset_cnt[module];
//		data.u32[1] =  _can_error_cnt[module];
//		// don't report error for BE because it happens when no one is listening
//		// and it will create an endless error-report-error loop
//		if (error != 2) can_send(module, 0, &data, 8);
#endif
	}
	
	if (icr.TI1 || icr.TI2 || icr.TI3)
	{
		hal_can_sent_handler(module);
	}
}

void can_flush(int module)
{
	_can_error_cnt[module]++;

   	CAN_MODULE *can = _can_table[module];
	// abort transmission in all three buffers
	can->CMR = CAN_CMR_RRB | CAN_CMR_AT | CAN_CMR_STB1 | CAN_CMR_STB2 | CAN_CMR_STB3;
}

void can_reset(int module)
{
	_can_reset_cnt[module]++;

   	CAN_MODULE *can = _can_table[module];
	can->MOD = 1;	// Reset
    can->CMR = CAN_CMR_RRB | CAN_CMR_AT | CAN_CMR_STB1 | CAN_CMR_STB2 | CAN_CMR_STB3;
	can->MOD = 4;	// STM = self test mode
}

int hal_can_send(CAN_EP ep, CAN_BUFFER *data, unsigned char length, CAN_MSG_FLAGS flags)
{
	int module = ep.Bus;
	if (_can_modules_enabled & (1<<module))
	{
		CAN_MODULE *can = _can_table[module];
	
		// wait for buffer ready
		CAN_TB *tb;
		unsigned long cmr;
		if (flags & (CANF_PRI_HIGH | CANF_PRI_MED | CANF_PRI_LOW))
		{
			while(1)
			{
				if ((flags & CANF_PRI_HIGH) &&
					(can->SRbits.TB1 & CAN_SR_TBS))
				{ 
					tb = &can->TB1;
					cmr = CAN_CMR_STB1;
					break; 
				}
				if ((flags & CANF_PRI_MED) &&
					(can->SRbits.TB2 & CAN_SR_TBS))
				{ 
					tb = &can->TB2; 
					cmr = CAN_CMR_STB2;
					break; 
				}
				if ((flags & CANF_PRI_LOW) &&
					(can->SRbits.TB3 & CAN_SR_TBS))
				{ 
					tb = &can->TB3; 
					cmr = CAN_CMR_STB3;
					break; 
				}
				if (can->GSRbits.ES)
				{
					can_reset(module);
					return 0;
				}
			}
		
			// Write DLC, RTR and FF
			tb->TFI = length << 16;
			tb->TID = ep.Id & 0x7FF;
			tb->TDA = data->u32[0];
			tb->TDB = data->u32[1];
		
			// Write transmission request
			can->CMR = CAN_CMR_TR | cmr;
			return 1;
		}
	}
	return 0;
}

unsigned long can_get_reset_count(int module)
{
	return _can_reset_cnt[module];
}

unsigned long can_get_error_count(int module)
{
	return _can_error_cnt[module];
}

int hal_fullcan_setup(HAL_FULLCAN_SETUP_CALLBACK callback, void *state)
{
	int count = 0;
	_fcan->AFMR = AFMR_AccBP;	// bypass acceptance filter and allow write to af_ram
	
	unsigned long *id_ptr = (unsigned long *)_af_ram;
	while(count < 64)
	{
		CAN_EP ep1, ep2;
		CAN_MSG_FLAGS flags1 = CANF_NONE;
		CAN_MSG_FLAGS flags2 = CANF_NONE;

		if (!callback(count, &ep1, &flags1, state))
			break;
		count++;

		if (!callback(count, &ep2, &flags2, state))
		{
			*id_ptr++ = (FCAN_MAKE_ID(ep1.Bus, ep1.Id, flags1 & CANF_RXINT) << 16)
				| FCAN_DISABLE;
			break;
		}
		else
		{
			*id_ptr++ = (FCAN_MAKE_ID(ep1.Bus, ep1.Id, flags1 & CANF_RXINT) << 16) 
				| FCAN_MAKE_ID(ep2.Bus, ep2.Id, flags2 & CANF_RXINT);
			count++;
		}
	}
	_fcan->SFF_sa = (unsigned long)id_ptr;
	_fcan->SFF_GRP_sa = _fcan->EFF_sa = _fcan->EFF_GRP_sa = _fcan->ENDofTable = _fcan->SFF_sa;

	FCAN_MSG *msg = (FCAN_MSG *)id_ptr;
	for (int i = 0; i < count; i++)
	{
		msg->Status = 0;
		msg++;
	}

	_fcan->FCANIE = 1;
	_fcan->AFMR = AFMR_eFCAN;
	return count;
}

int hal_fullcan_read_msg(int index, CAN_MSG *msg)
{
	FCAN_MSG *msg_table = (FCAN_MSG *)(_af_ram + _fcan->ENDofTable);
	while(1)
	{
		unsigned long status = msg_table[index].Status;
		int sem = (status & FCAN_MSG_SEM_MASK) >> FCAN_MSG_SEM_BIT;
		if (sem != 1)
		{
			msg_table[index].Status = status & ~FCAN_MSG_SEM_MASK;
			FCAN_MSG temp = msg_table[index];
			if (msg_table[index].StatusBits.SEM == 0)
			{
				msg->EP = (CAN_EP) { .Id = temp.StatusBits.ID_High, .Bus = temp.StatusBits.SCC };
				msg->Length = temp.StatusBits.DLC;
				msg->Data = temp.Data;
				msg->Flags = 0;
				return (sem == 3);
			}
		}
	}
}


