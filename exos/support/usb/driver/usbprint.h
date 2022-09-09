#ifndef USB_DRIVER_USBPRINT_H
#define USB_DRIVER_USBPRINT_H

#include <usb/classes/usbprint.h>
#include <usb/host.h>
#include <kernel/tree.h>
#include <kernel/io.h>

#define USBPRINT_USB_BUFFER 64

typedef enum
{
	USBPRINT_NOT_ATTACHED = 0,
	USBPRINT_ATTACHING,
	USBPRINT_CLOSED,
	USBPRINT_OPENING,
	USBPRINT_READY,
	USBPRINT_ERROR,
} usbprint_state_t;

typedef struct
{
	usb_host_function_t;
	unsigned char Protocol;
	unsigned char Interface;

	usb_host_pipe_t BulkInputPipe;
	usb_host_pipe_t BulkOutputPipe;
	usbprint_state_t State;

	io_entry_t *Entry;
	io_tree_device_t KernelDevice;
	unsigned char Buffer[USBPRINT_USB_BUFFER];
} usbprint_function_t;


void usbprint_initialize(unsigned short vendor_id, unsigned short product_id);


#endif // USB_DRIVER_USBPRINT_H
