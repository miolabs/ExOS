#ifndef USB_DRIVER_USBPRINT
#define USB_DRIVER_USBPRINT

#include <usb/classes/usbprint.h>
#include <usb/host.h>
#include <kernel/tree.h>
#include <comm/comm.h>

#define USBPRINT_BUFFER_SIZE 64

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
	EXOS_TREE_DEVICE KernelDevice;
	unsigned char Buffer[USBPRINT_BUFFER_SIZE];

	unsigned char Interface;
	unsigned char Protocol;

} USBPRINT_FUNCTION;


void usbprint_initialize();


#endif // USB_DRIVER_USBPRINT
