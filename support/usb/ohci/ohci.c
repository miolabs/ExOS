#include "ohci.h"
#include "pipes.h"
#include "buffers.h"
#include "hub.h"
#include <kernel/panic.h>

// Host Controller Communications Area (must be 256-aligned)
static volatile OHCI_HCCA _hcca __usb __attribute__((aligned(256)));
static EXOS_EVENT _sof_event;
static void _soft_reset();

int ohci_initialize()
{
	ohci_hub_initialize();
	exos_event_create(&_sof_event);

	_soft_reset();

	__hc->Control = 0;
	__hc->ControlBits.HCFS = OHCI_OP_CONTROL_FS_OPERATIONAL;

	int fi = 12000 - 1;
	__hc->FmInterval = (((fi - 210) * 6 / 7) << 16) | fi;
	__hc->PeriodicStart = (fi * 90 / 100);	// 10% of bandwidth reserved

	// initialize HCCA
	for (int i = 0; i < 32; i++) _hcca.IntTable[i] = 0;
	_hcca.FrameNumber = 0;
	_hcca.DoneHead = 0;

    __hc->HCCA = &_hcca;

	// enable list processing
	__hc->Control |= OHCIR_CONTROL_CLE | OHCIR_CONTROL_BLE | OHCIR_CONTROL_PLE | OHCIR_CONTROL_IE;

	// Set Global Power
	__hc->RhStatus = OHCIR_RH_STATUS_LPSC;

	// enable interrupts
	__hc->InterruptEnable = OHCIR_INTR_ENABLE_MIE | OHCIR_INTR_ENABLE_UE | OHCIR_INTR_ENABLE_SO |
				OHCIR_INTR_ENABLE_WDH | OHCIR_INTR_STATUS_SF |
				OHCIR_INTR_ENABLE_RHSC;
}

static void _soft_reset()
{
	// sw reset
	__hc->CommandStatus = OHCIR_CMD_STATUS_HCR;
    while(__hc->CommandStatus & OHCIR_CMD_STATUS_HCR);
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

// NOTE: this is called by lower USB host/otg layer
void ohci_isr()
{
	int int_status = __hc->InterruptStatus & __hc->InterruptEnable;
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
			while(hctd = (OHCI_HCTD *)(_hcca.DoneHead & ~0x1), hctd != NULL)
			{
				OHCI_HCTD *prev = NULL;
				while(hctd->Next != NULL) 
				{
					prev = hctd;
					hctd = hctd->Next;
				}
				
				OHCI_STD *std = (OHCI_STD *)hctd;
				std->Status = (hctd->ControlBits.ConditionCode == OHCI_TD_CC_NO_ERROR) ?
					OHCI_STD_STA_COMPLETED : OHCI_STD_STA_ERROR;

				USB_REQUEST_BUFFER *urb = std->Request;
				if (urb == NULL) 
					kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
				if (std->Status == OHCI_STD_STA_COMPLETED &&
					std->HCTD.ControlBits.ConditionCode == OHCI_TD_CC_NO_ERROR)
				{
					// NOTE: stds have rounding option always set (partial transfers are NOT errors)
					// if buffer was entirely transferred, CurrBufPtr will be NULL 
					urb->Done = (std->HCTD.CurrBufPtr == NULL) ? urb->Length : (std->HCTD.CurrBufPtr - urb->Data);
					urb->Status = URB_STATUS_DONE;
				}
				else
				{
					urb->Status = URB_STATUS_FAILED;
				}
				exos_event_set(&urb->Event);
				
				if (prev == NULL) break;
				prev->Next = NULL;
			}
		}

        if (int_status & OHCIR_INTR_STATUS_SF)
			exos_event_reset(&_sof_event);

		// clear interrupt flags 
		__hc->InterruptStatus = int_status;
	}      
}

unsigned short ohci_get_current_frame()
{
	return __hc->FmNumber;
}

void ohci_wait_sof()
{
	exos_event_wait(&_sof_event, EXOS_TIMEOUT_NEVER);
}

