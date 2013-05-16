#include "pipes.h"
#include "buffers.h"
#include <kernel/thread.h>
#include <kernel/panic.h>

void ohci_pipe_add(USB_HOST_PIPE *pipe)
{
	OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;
	switch(pipe->EndpointType)
	{
		case USB_TT_CONTROL:	
			sed->HCED.Next = _hc->ControlHeadED;
			_hc->ControlHeadED = sed;
			break;
		case USB_TT_BULK:
			sed->HCED.Next = _hc->BulkHeadED;
			_hc->BulkHeadED = sed;
			break;
	}
}

void ohci_pipe_remove(USB_HOST_PIPE *pipe)
{
	OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;
	// FIXME: look for the matching pipe pointer for removing
	OHCI_HCED **hced;
	switch(pipe->EndpointType)
	{
		case USB_TT_CONTROL: 
			hced = (OHCI_HCED **)&_hc->ControlHeadED;
			_hc->ControlBits.CLE = 0;
			break;
		case USB_TT_BULK:
			hced = (OHCI_HCED **)&_hc->BulkHeadED;
			_hc->ControlBits.BLE = 0;
			break;
		default:
			// TODO: Remove Interrupt/Iso EDs
			return;
	}
	
	// Wait current frame to end to be sure that endpoint is not in use
	exos_thread_sleep(1); // FIXME: we should wait for EOF
	
	sed->HCED.ControlBits.sKip = 1;
	while(*hced != NULL)
	{
		if (*hced == (OHCI_HCED *)sed)
		{
			// remove ed 
			*hced = (OHCI_HCED *)sed->HCED.Next;
			break;
		}
		else
		{
			// TODO
		}
	}
}


static int _bandwidth(OHCI_HCED *hced)
{
	int total = 0;
	while(hced != NULL)
	{
		total += hced->ControlBits.MaxPacketSize;
		// FIXME: maybe we should sum here all TDs instead of supposing a single (of max packet size) TD 
		hced = (OHCI_HCED *)hced->Next;
	}
	return total;
}

void ohci_pipe_schedule(USB_HOST_PIPE *pipe)
{
	OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;

	int best_index = 0;
	int period = pipe->EndpointType == USB_TT_INTERRUPT ? pipe->InterruptInterval : 1;

	if (period > 1)	
	{
		// get lowest bandwidth index
		int min_bandwidth = 0;
		int max_bandwidth = 0;
		for(int i = 0; i < 32; i++)
		{
			OHCI_HCED *hced = *ohci_get_periodic_ep(i);
			int bandwidth = _bandwidth(hced);
			if (i == 0 || bandwidth < min_bandwidth)
			{
				best_index = i;
				min_bandwidth = bandwidth;
			}
			if (bandwidth > max_bandwidth)
			{
				max_bandwidth = bandwidth;
			}
		}
	}

// TODO
/* 
	// insert at specified interval in periodic lists
	for(int ms = 0; ms < 32; ms += period)
	{
		int index = (ms + best_index) & 31;
		OHCI_PIPE **p_ptr = (OHCI_PIPE **)ohci_get_periodic_ep(index);
		OHCI_PIPE *link_pipe = NULL;

		while(*p_ptr != NULL)
		{
			link_pipe = *p_ptr; 
			if (pipe->EndpointType > link_pipe->EndpointType) break; // NOTE: TT_INTERRUPT is bigger than TT_ISO
			if (period >= link_pipe->InterruptInterval) break;
			p_ptr = (OHCI_PIPE **)&link_pipe->HCED.Next;
		}

		if (link_pipe != pipe)	// don't queue again!
		{
			pipe->HCED.Next = (OHCI_HCED *)*p_ptr;
			*p_ptr = pipe;
		}
	}
*/
}

static void _init_hctd(OHCI_HCTD *hctd, OHCI_TD_PID pid, OHCI_TD_TOGGLE td_toggle, void *buffer, int length)
{
	hctd->ControlBits.Pid = pid;
	hctd->ControlBits.Rounding = 1;
	hctd->ControlBits.DelayInterrupt = 0;
	hctd->ControlBits.Toggle = td_toggle;
	hctd->ControlBits.ErrorCount = 0;
	hctd->ControlBits.ConditionCode = 0xF;
	
	hctd->CurrBufPtr = buffer;
	hctd->BufferEnd = buffer + (length - 1);
}

OHCI_STD *ohci_add_std(USB_REQUEST_BUFFER *urb, OHCI_STD *next_std, OHCI_TD_PID pid, OHCI_TD_TOGGLE toggle)
{
#ifdef DEBUG
	if (urb == NULL || urb->Pipe == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	USB_HOST_PIPE *pipe = urb->Pipe;
	OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;
	OHCI_STD *std = NULL;
	OHCI_STD *tail_std = (OHCI_STD *)sed->HCED.TailTD;
	if (tail_std != NULL)
	{
		if (next_std == NULL)
		{
			next_std = ohci_buffers_alloc_std();
			if (next_std == NULL) return NULL;
		}
		std = tail_std;
		_init_hctd(&std->HCTD, pid, toggle, urb->Data, urb->Length);
		std->Request = urb; 
		std->Status = OHCI_STD_STA_READY;
		
		exos_event_reset(&urb->Event);
		urb->State = std;
		urb->Status = URB_STATUS_ISSUED;
		
		OHCI_HCTD *new_hctd = &next_std->HCTD;
		ohci_clear_hctd(new_hctd);
		tail_std->HCTD.Next = new_hctd;

		sed->HCED.TailTD = new_hctd;

		switch (pipe->EndpointType)
		{
			case USB_TT_CONTROL:
				_hc->CommandStatus |= OHCIR_CMD_STATUS_CLF;
				break;
			case USB_TT_BULK:
				_hc->CommandStatus |= OHCIR_CMD_STATUS_BLF;
				break;
		}
	}
	return std;
}

int ohci_process_std(USB_REQUEST_BUFFER *urb, OHCI_TD_PID pid, OHCI_TD_TOGGLE toggle, void *data, int length)
{
#ifdef DEBUG
	if (urb == NULL || urb->Pipe == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	urb->Data = data;
	urb->Length = length;
	OHCI_STD *std = ohci_add_std(urb, NULL, pid, toggle);
	if (std != NULL)
	{
		exos_event_wait(&urb->Event, EXOS_TIMEOUT_NEVER);	// FIXME: support timeouts
		ohci_buffers_release_std(std);
	}
	if (urb->Status == URB_STATUS_DONE) return 1;
	return 0;
}


