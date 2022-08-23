#ifndef USB_DRIVER_USBHUB_H
#define USB_DRIVER_USBHUB_H

#include <usb/classes/hub.h>
#include <usb/host.h>
#include <kernel/dispatch.h>

typedef struct
{
	usb_host_function_t;
	usb_host_pipe_t InputPipe;
	unsigned char Interface;
	unsigned char ReportSize;
	unsigned char PortCount;
	unsigned char PwrOn2PwrGood;

	unsigned char InputBuffer[64];
	
	unsigned char ExitFlag;
	unsigned char InstanceIndex;
	unsigned short HubCharacteristics;

	usb_request_buffer_t Request;
	dispatcher_t Dispatcher;
	list_t Children;
	event_t ExitEvent;
} usb_hub_function_t;

#define USB_HUB_CHARF_LOGICAL_POWER_MASK (0x3 << 0)
#define USB_HUB_CHARF_LOGICAL_POWER_GANGED (0x0 << 0)
#define USB_HUB_CHARF_LOGICAL_POWER_INDIVIDUAL (0x1 << 0)
#define USB_HUB_CHARF_COMPOUND_DEVICE (1 << 2)
#define USB_HUB_CHARF_OVERCURRENT_MODE_MASK (0x3 << 3)
#define USB_HUB_CHARF_OVERCURRENT_MODE_GLOBAL (0x0 << 3)
#define USB_HUB_CHARF_OVERCURRENT_MODE_INDIVIDUAL (0x1 << 3)
#define USB_HUB_CHARF_PORT_INDICATORS_SUPPORTED (1 << 7)


void usb_hub_initialize(dispatcher_context_t *context);

#endif // USB_DRIVER_USBHUB_H


