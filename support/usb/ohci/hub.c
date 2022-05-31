#include "hub.h"
#include "driver.h"
#include <usb/enumerate.h>
#include <kernel/thread.h>
#include <kernel/dispatch.h>
#include <support/services/debug.h>

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));

static EXOS_EVENT _root_event;
static EXOS_DISPATCHER_CONTEXT _context;

static USB_HOST_DEVICE _devices[USB_HOST_ROOT_HUB_NDP] __usb;

static void *_service(void *arg);

void ohci_hub_initialize()
{
	exos_event_create(&_root_event);
	exos_dispatcher_context_create(&_context);

	EXOS_EVENT event;
	exos_event_create(&event);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, NULL, _service, &event);
	exos_event_wait(&event, EXOS_TIMEOUT_NEVER);
}

void ohci_hub_signal()
{
	exos_event_set(&_root_event);
}

static void _dispatch(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	exos_event_reset(&_root_event);

	exos_thread_sleep(500);

	for(int port = 0; port < USB_HOST_ROOT_HUB_NDP; port++)
	{
		int status = __hc->RhPortStatus[port];
		if (status & OHCIR_RH_PORT_CSC)	// status changed
		{
			__hc->RhPortStatus[port] = OHCIR_RH_PORT_CSC;
			if (status & OHCIR_RH_PORT_CCS) 
			{
				exos_thread_sleep(50);	// at least 50ms before reset as of USB 2.0 spec
				
				__hc->RhPortStatus[port] = OHCIR_RH_PORT_PRS;	// assert reset
				while(__hc->RhPortStatus[port] & OHCIR_RH_PORT_PRS)
					exos_thread_sleep(1);
				exos_thread_sleep(100);	// some devices need up to 100ms after port reset
				
				__hc->RhPortStatus[port] = OHCIR_RH_PORT_PRSC;	// clear prsc
				USB_HOST_DEVICE_SPEED speed = (__hc->RhPortStatus[port] & OHCIR_RH_PORT_LSDA) ? 
					USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
				
				USB_HOST_DEVICE *child = &_devices[port];
				ohci_device_create(child, port, speed);
				debug_printf("usb_roothub: child %04x/%04x added at port #%d\r\n", child->Vendor, child->Product, child->Port);
			}
			else 
			{
				USB_HOST_DEVICE *child = &_devices[port];
				debug_printf("usb_roothub: child %04x/%04x removing at port #%d\r\n", child->Vendor, child->Product, child->Port);
				ohci_device_destroy(child);
				debug_printf("usb_roothub: child %04x/%04x removed\r\n", child->Vendor, child->Product);
			}
		}
		if (status & OHCIR_RH_PORT_PRSC)
		{
			__hc->RhPortStatus[port] = OHCIR_RH_PORT_PRSC;
		}
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

static void *_service(void *arg)
{
	EXOS_DISPATCHER dispatcher;
	exos_dispatcher_create(&dispatcher, &_root_event, _dispatch, NULL);
	exos_dispatcher_add(&_context, &dispatcher, EXOS_TIMEOUT_NEVER);
	exos_event_set(&_root_event);

    exos_event_set((EXOS_EVENT *)arg);    // notify of service ready

	while(1)
	{
		exos_dispatch(&_context, EXOS_TIMEOUT_NEVER);
	}
}


