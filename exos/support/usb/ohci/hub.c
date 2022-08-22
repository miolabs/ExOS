#include "hub.h"
#include "driver.h"
#include <usb/enumerate.h>
#include <kernel/thread.h>
#include <kernel/dispatch.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define THREAD_STACK 2048
static exos_thread_t _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));

static event_t _root_event;
static dispatcher_context_t _context;
static usb_host_controller_t *_hc_driver;

static void *_service(void *arg);

void ohci_hub_initialize(usb_host_controller_t *hc)
{
	ASSERT(hc != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(_hc_driver == nullptr, KERNEL_ERROR_KERNEL_PANIC);	// already in use
	_hc_driver = hc;
	
	exos_event_create(&_root_event, EXOS_EVENTF_AUTORESET);
	exos_dispatcher_context_create(&_context);

	event_t event;
	exos_event_create(&event, EXOS_EVENTF_AUTORESET);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, _service, &event);
	exos_event_wait(&event, EXOS_TIMEOUT_NEVER);
}

void ohci_hub_signal()
{
	exos_event_set(&_root_event);
}

static void _dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
//	exos_event_reset(&_root_event);

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
				usb_host_device_state_t speed = (__hc->RhPortStatus[port] & OHCIR_RH_PORT_LSDA) ? 
					USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
				
				usb_host_device_t *child = usb_host_create_root_device(_hc_driver, port, speed);
				if (child != nullptr)
					verbose(VERBOSE_DEBUG, "usb_roothub", "child %04x/%04x added at port #%d", child->Vendor, child->Product, child->Port);
				else	
					verbose(VERBOSE_DEBUG, "usb_roothub", "device add failed");				}
			else 
			{
				usb_host_device_t *child = &_hc_driver->Devices[port];
				verbose(VERBOSE_COMMENT, "usb_roothub", "child %04x/%04x removing at port #%d\r\n", child->Vendor, child->Product, child->Port);
				usb_host_destroy_device(child);
				verbose(VERBOSE_COMMENT, "usb_roothub", "child %04x/%04x removed\r\n", child->Vendor, child->Product);
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
	dispatcher_t dispatcher;
	exos_dispatcher_create(&dispatcher, &_root_event, _dispatch, NULL);
	exos_dispatcher_add(&_context, &dispatcher, EXOS_TIMEOUT_NEVER);
	exos_event_set(&_root_event);

    exos_event_set((event_t *)arg);    // notify of service ready

	while(1)
	{
		exos_dispatch(&_context, EXOS_TIMEOUT_NEVER);
	}
}


