#ifndef USB_MODEM_H
#define USB_MODEM_H

#include <usb/classes/usbprint.h>
#include <usb/host.h>
#include <kernel/tree.h>
#include <kernel/dispatch.h>
#include <comm/comm.h>

#define USBMODEM_USB_BUFFER 64
#define USBMODEM_IO_BUFFER 512

typedef enum
{
	USBMODEM_NOT_ATTACHED = 0,
	USBMODEM_ATTACHING_PRE,
	USBMODEM_SWITCHING,
	USBMODEM_ATTACHING,
	USBMODEM_CLOSED,
	USBMODEM_OPENING,
	USBMODEM_READY,
	USBMODEM_ERROR,
} USBMODEM_STATE;

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
	USB_HOST_PIPE InterruptPipe;
	USBMODEM_STATE State;

	unsigned char OutputBuffer[USBMODEM_USB_BUFFER];	// used for output and setup
	unsigned char InputBuffer[USBMODEM_USB_BUFFER];

	COMM_IO_ENTRY *Entry;
	EXOS_TREE_DEVICE KernelDevice;

	unsigned char Interface;
//	unsigned char Protocol;

	USB_REQUEST_BUFFER Request;
	EXOS_IO_BUFFER IOBuffer;
	EXOS_DISPATCHER Dispatcher;
} USBMODEM_FUNCTION;

typedef struct
{
	USBMODEM_FUNCTION *Function;
	EXOS_EVENT DoneEvent; 
} USBMODEM_REQUEST;

void usbmodem_initialize();


#endif // USB_MODEM_H

