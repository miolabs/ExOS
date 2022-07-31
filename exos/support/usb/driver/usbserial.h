#ifndef USB_DRIVER_USBSERIAL
#define USB_DRIVER_USBSERIAL

#include <usb/classes/cdc.h>
#include <usb/host.h>
#include <kernel/tree.h>
#include <comm/comm.h>
#include <kernel/dispatch.h>

#define USBSERIAL_USB_BUFFER 64
#define USBSERIAL_IO_BUFFER 512

typedef enum
{
	USBSERIAL_NOT_ATTACHED = 0,
	USBSERIAL_ATTACHING,
	USBSERIAL_CLOSED,
	USBSERIAL_OPENING,
	USBSERIAL_READY,
	USBSERIAL_ERROR,
} USBSERIAL_STATE;

typedef struct
{
	USB_HOST_FUNCTION;
        USB_HOST_DEVICE Dev;
	USB_HOST_PIPE BulkInputPipe;
	USB_HOST_PIPE BulkOutputPipe;
        USB_HOST_PIPE InterruptPipe;
	USBSERIAL_STATE State;

	COMM_IO_ENTRY *Entry;
	EXOS_TREE_DEVICE KernelDevice;
	unsigned char Buffer[USBSERIAL_USB_BUFFER];
        unsigned char BufferIn[USBSERIAL_USB_BUFFER];


	unsigned char Interface;
	
        USB_REQUEST_BUFFER Request;
	//EXOS_IO_BUFFER IOBuffer;
	//EXOS_DISPATCHER Dispatcher;

} USBSERIAL_FUNCTION;


void usbserial_initialize(unsigned short vendor_id, unsigned short product_id);


#endif // USB_DRIVER_USBPRINT
