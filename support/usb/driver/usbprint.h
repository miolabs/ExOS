#ifndef USB_DRIVER_USBPRINT
#define USB_DRIVER_USBPRINT

#include <usb/classes/usbprint.h>
#include <usb/host.h>
#include <kernel/tree.h>
#include <comm/comm.h>

#define USBPRINT_USB_BUFFER 64

typedef enum
{
	USBPRINT_NOT_ATTACHED = 0,
	USBPRINT_ATTACHING,
	USBPRINT_CLOSED,
	USBPRINT_OPENING,
	USBPRINT_READY,
	USBPRINT_ERROR,
} USBPRINT_STATE;

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
	USBPRINT_STATE State;

	COMM_IO_ENTRY *Entry;
	EXOS_TREE_DEVICE KernelDevice;
	unsigned char Buffer[USBPRINT_USB_BUFFER];

	unsigned char Interface;
	unsigned char Protocol;

} USBPRINT_FUNCTION;


void usbprint_initialize();


#endif // USB_DRIVER_USBPRINT
