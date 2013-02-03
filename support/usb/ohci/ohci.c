#include "ohci.h"
#include "pipes.h"
#include "buffers.h"
#include "hub.h"
#include <kernel/panic.h>

// Host Controller Communications Area (must be 256-aligned)
static OHCI_HCCA _hcca __usb;

static void _soft_reset();

int ohci_initialize()
{
	ohci_hub_initialize();

	_soft_reset();

	_hc->Control = 0;
	_hc->ControlBits.HCFS = OHCI_OP_CONTROL_FS_OPERATIONAL;

	int fi = 12000 - 1;
	_hc->FmInterval = (((fi - 210) * 6 / 7) << 16) | fi;
	_hc->PeriodicStart = (fi * 90 / 100);	// 10% of bandwidth reserved

	// initialize HCCA
	for (int i = 0; i < 32; i++) _hcca.IntTable[i] = 0;
	_hcca.FrameNumber = 0;
	_hcca.DoneHead = 0;

    _hc->HCCA = &_hcca;

	// enable list processing
	_hc->Control |= OHCIR_CONTROL_CLE | OHCIR_CONTROL_BLE | OHCIR_CONTROL_PLE | OHCIR_CONTROL_IE;

	// Set Global Power
	_hc->RhStatus = OHCIR_RH_STATUS_LPSC;

	// enable interrupts
    _hc->InterruptEnable = OHCIR_INTR_ENABLE_MIE | OHCIR_INTR_ENABLE_UE | OHCIR_INTR_ENABLE_SO |
                         OHCIR_INTR_ENABLE_WDH |
                         OHCIR_INTR_ENABLE_RHSC;
}

static void _soft_reset()
{
	// sw reset
	_hc->CommandStatus = OHCIR_CMD_STATUS_HCR;
    while(_hc->CommandStatus & OHCIR_CMD_STATUS_HCR);
}

OHCI_HCED **ohci_get_periodic_ep(int index)
{
	return (OHCI_HCED **)&(_hcca.IntTable[index]);
}

void ohci_clear_hced(OHCI_HCED *hced)
{
    hced->Control = 0;
    hced->TailTD = 0;
    hced->HeadTD = 0;
    hced->Next = 0;
}

void ohci_clear_hctd(OHCI_HCTD *hctd)
{
	hctd->Control = 0;
	hctd->CurrBufPtr = 0;
	hctd->Next = 0;
	hctd->BufferEnd = 0;
}

void ohci_clear_hctd_iso(OHCI_HCTD_ISO *isotd)
{
	isotd->Control = 0;
	isotd->Next = 0;
	isotd->BP0 = isotd->BufferEnd = 0;
	for(int i = 0; i < 8; i++) isotd->Offset[i] = 0;
}

int ohci_init_hctd_iso(OHCI_HCTD_ISO *itd, int sf, void *buffer, int length, int packet_size)
{
	unsigned long addr =  (unsigned long)buffer;
	itd->BP0 = (void*)(addr & 0xFFFFF000);

	int rem = length;
	int r = 0;
	while(r < 8)
	{
		itd->Offset[r] = (OHCI_TD_CC_NOT_ACCESSED << 12) | (addr & 0xFFF);
		if (rem > packet_size)
		{
			rem -= packet_size;
			addr += packet_size;
			r++;
		}
		else
		{
			addr += (rem - 1);
			break;
		}
	}
	itd->BufferEnd = (void *)addr;

	itd->ControlBits.ConditionCode = -1;	// condition code
	itd->ControlBits.FrameCount = r;		// frame count
	itd->ControlBits.DelayInterrupt = 0;	// delay interrupt
	itd->ControlBits.StartingFrame = sf;
	return r + 1;
}

// NOTE: this is called by lower USB host otg layer
void ohci_isr()
{
	int int_status = _hc->InterruptStatus & _hc->InterruptEnable;
	if (int_status != 0)
	{
		if ((int_status & OHCIR_INTR_STATUS_UE) ||
			(int_status & OHCIR_INTR_STATUS_SO))
		{
			_soft_reset();
		}

		if (int_status & OHCIR_INTR_STATUS_RHSC)	// Root Hub Status Change interrupt
		{
			ohci_hub_signal();
		}

		if (int_status & OHCIR_INTR_STATUS_WDH)		// Writeback Done Head interrupt
		{
			// We can process the Done Queue here
			// Host Controller will not try to access the HccaDoneHead until we clear the WDH bit in HcInterruptStatus

			OHCI_HCTD *hctd;
			while(NULL != (hctd = (OHCI_HCTD *)_hcca.DoneHead))
			{
				OHCI_HCTD *prev = NULL;
				while(hctd->Next != NULL) 
				{
					prev = hctd;
					hctd = (OHCI_HCTD *)hctd->Next;
				}
				
				OHCI_STD *std = (OHCI_STD *)hctd;
				std->Status = (hctd->ControlBits.ConditionCode == OHCI_TD_CC_NO_ERROR) ?
					OHCI_STD_STA_COMPLETED : OHCI_STD_STA_ERROR;

				USB_HOST_PIPE *pipe = std->Pipe;
				if (pipe == NULL) 
					kernel_panic(KERNEL_ERROR_NULL_POINTER);
				exos_event_set(&pipe->Event);

				if (prev == NULL) break;
				prev->Next = NULL;
			}
		}
		// clear interrupt flags 
		_hc->InterruptStatus = int_status;
	}      
}
















unsigned short ohci_get_current_frame()
{
	return _hc->FmNumber;
}


