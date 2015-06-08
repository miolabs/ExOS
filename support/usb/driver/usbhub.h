#ifndef USB_DRIVER_HUB_H
#define USB_DRIVER_HUB_H

#include <usb/classes/hub.h>
#include <usb/host.h>
#include <kernel/dispatch.h>

typedef struct __HID_FUNCTION_HANDLER HID_FUNCTION_HANDLER;

typedef struct
{
	USB_HOST_FUNCTION;
	USB_HOST_PIPE InputPipe;
	unsigned char Reserved;
	unsigned char Interface;
	unsigned char ReportSize;
	unsigned char PortCount;

	unsigned char InputBuffer[64];
	
	unsigned char ExitFlag;
	unsigned char StartedFlag;
	unsigned char InstanceIndex;

	USB_REQUEST_BUFFER Urb;
	EXOS_DISPATCHER Dispatcher;
	EXOS_LIST Children;
	EXOS_EVENT ExitEvent;
} USB_HUB_FUNCTION;

// prototypes
void usbd_hub_initialize();

#endif // USB_DRIVER_HUB_H


