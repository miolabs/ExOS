#ifndef OHCI_PIPES_H
#define OHCI_PIPES_H

#include <support/usb/ohci/ohci.h>
#include <usb/host.h>

typedef struct __attribute__((aligned(16)))
{
	OHCI_HCED HCED;
	USB_HOST_PIPE *Pipe;
} OHCI_SED;

typedef enum
{
	OHCI_STD_STA_FREE = 0,
	OHCI_STD_STA_EMPTY,
	OHCI_STD_STA_READY,
	OHCI_STD_STA_COMPLETED,
	OHCI_STD_STA_ERROR,
	OHCI_STD_STA_CANCELLED,
} OHCI_STD_STATUS;

typedef struct __attribute__((aligned(32))) _OHCI_STD
{
	union // this field must be the first one to allow pointer casting
	{
		OHCI_HCTD HCTD;
		OHCI_HCTD_ISO HCTD_ISO;
	};
	USB_REQUEST_BUFFER *Request;
	volatile OHCI_STD_STATUS Status;
} OHCI_STD;

// prototypes
void ohci_pipe_add(USB_HOST_PIPE *pipe);
void ohci_pipe_remove(USB_HOST_PIPE *pipe);
void ohci_pipe_schedule(USB_HOST_PIPE *pipe);
int ohci_pipe_flush(USB_HOST_PIPE *pipe, USB_REQUEST_BUFFER *urb);

OHCI_STD *ohci_add_std(USB_REQUEST_BUFFER *urb, OHCI_STD *next_std, OHCI_TD_PID pid, OHCI_TD_TOGGLE toggle);
int ohci_remove_std(USB_REQUEST_BUFFER *urb);
int ohci_process_std(USB_REQUEST_BUFFER *urb, OHCI_TD_PID pid, OHCI_TD_TOGGLE toggle, void *data, int length);

#endif // OHCI_PIPES_H
