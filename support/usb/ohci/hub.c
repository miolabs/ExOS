#include "hub.h"
#include "driver.h"
#include <usb/enumerate.h>
#include <kernel/thread.h>
#include <kernel/event.h>

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static int _hub_sigmask = 0;

static USB_DEVICE_DESCRIPTOR _dev_desc __usb;	// buffer for descriptors
static volatile int _last_device = 0;

static void *_service(void *arg);

void ohci_hub_initialize()
{
	EXOS_EVENT event;
	exos_event_create(&event);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, NULL, _service, &event);
	exos_event_wait(&event, EXOS_TIMEOUT_NEVER);
}

void ohci_hub_signal()
{
	exos_signal_set(&_thread, _hub_sigmask);
}

static int _enumerate(int port, USB_HOST_DEVICE_SPEED speed)
{
	USB_HOST_DEVICE *device = ohci_device_create(port, speed);
	USB_HOST_PIPE *pipe = &device->ControlPipe;
	USB_DEVICE_DESCRIPTOR *dev_desc = &_dev_desc;

	// read device descriptor (header)
	int done = usb_host_read_device_descriptor((USB_HOST_DEVICE *)device, USB_DESCRIPTOR_TYPE_DEVICE, 0, dev_desc, 8);
	if (done)
	{
		OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;
		sed->HCED.ControlBits.MaxPacketSize = dev_desc->MaxPacketSize;
		
		// set address
		device->Address = ++_last_device;	// FIXME: search lowest unused
		done = usb_host_set_address((USB_HOST_DEVICE *)device, device->Address);
		if (done)
		{
			sed->HCED.ControlBits.FunctionAddress = device->Address;

			// read device descriptor (complete) 
			done = usb_host_read_device_descriptor((USB_HOST_DEVICE *)device, USB_DESCRIPTOR_TYPE_DEVICE, 0, 
				dev_desc, sizeof(USB_DEVICE_DESCRIPTOR));
			
			if (done)
			{
				done = usb_host_enumerate((USB_HOST_DEVICE *)device, dev_desc); 
			}
		}
	}
    
	if (!done)
	{
		// TODO: destroy device
	}
    return done;
}



static void *_service(void *arg)
{
	_hub_sigmask = 1 << exos_signal_alloc();
	exos_event_set((EXOS_EVENT *)arg);	// notify of service ready

	while(1)
	{
		exos_signal_wait(_hub_sigmask, EXOS_TIMEOUT_NEVER);

		exos_thread_sleep(500);

		for(int port = 0; port < USB_HOST_ROOT_HUB_NDP; port++)
		{
			int status = _hc->RhPortStatus[port];
			if (status & OHCIR_RH_PORT_CSC)	// status changed
			{
				_hc->RhPortStatus[port] = OHCIR_RH_PORT_CSC;
				if (status & OHCIR_RH_PORT_CCS) 
				{
                	exos_thread_sleep(50);	// at least 50ms before reset as of USB 2.0 spec
					_hc->RhPortStatus[port] = OHCIR_RH_PORT_PRS;	// assert reset
					
					while(_hc->RhPortStatus[port] & OHCIR_RH_PORT_PRS)
						exos_thread_sleep(1);
					
					exos_thread_sleep(100);	// some devices need up to 100ms after port reset
					
					_hc->RhPortStatus[port] = OHCIR_RH_PORT_PRSC;	// clear prsc
					USB_HOST_DEVICE_SPEED speed = (_hc->RhPortStatus[port] & OHCIR_RH_PORT_LSDA) ? 
						USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
					
					_enumerate(port, speed);			
				}
				else 
				{
					// TODO: remove
				}
			}
			if (status & OHCIR_RH_PORT_PRSC)
			{
				_hc->RhPortStatus[port] = OHCIR_RH_PORT_PRSC;
//				if (status & OHCIR_RH_PORT_CCS) 
//				{
//					USB_HOST_DEVICE_SPEED speed = (_hc->RhPortStatus[port] & OHCIR_RH_PORT_LSDA) ? 
//						USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
//					
//					exos_thread_sleep(100);	// some devices need up to 100ms after port reset
//					_enumerate(port, speed);
//				}
			}
		}
	}
}
