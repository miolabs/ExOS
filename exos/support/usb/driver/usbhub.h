#ifndef USB_DRIVER_USBHUB_H
#define USB_DRIVER_USBHUB_H

#include <usb/classes/hub.h>
#include <usb/host.h>
#include <kernel/dispatch.h>

typedef struct
{
	usb_host_function_t;
	usb_host_pipe_t InputPipe;
	unsigned char Reserved;
	unsigned char Interface;
	unsigned char ReportSize;
	unsigned char PortCount;

	unsigned char InputBuffer[64];
	
	unsigned char ExitFlag;
//	unsigned char StartedFlag;
	unsigned char InstanceIndex;

	usb_request_buffer_t Request;
	dispatcher_t Dispatcher;
	list_t Children;
	event_t ExitEvent;
} usb_hub_function_t;

void usb_hub_initialize(dispatcher_context_t *context);

#endif // USB_DRIVER_USBHUB_H


