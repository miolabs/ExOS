#include "driver.h"
#include "buffers.h"
#include <usb/enumerate.h>
#include <kernel/fifo.h>
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

static bool _ctrl_setup_read(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *in_data, unsigned in_length);
static bool _ctrl_setup_write(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *out_data, unsigned out_length);
static bool _start_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
static bool _stop_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
static bool _begin_bulk_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, void *data, unsigned length);
static int _end_bulk_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, unsigned timeout);
static bool _create_device(usb_host_controller_t *hc, usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed);
static void _destroy_device(usb_host_controller_t *hc, usb_host_device_t *device);

static const usb_host_controller_driver_t _driver = {
	_ctrl_setup_read, _ctrl_setup_write, 
	_start_pipe, _stop_pipe, _begin_bulk_transfer, _end_bulk_transfer,
	_create_device, _destroy_device };

static usb_host_controller_t _hc;
static usb_host_device_t _root_devices[USB_HOST_ROOT_HUB_NDP] __usb;
static mutex_t _mutex;

void ohci_driver_initialize()
{
	exos_mutex_create(&_mutex);
	usb_host_controller_create(&_hc, &_driver, _root_devices, USB_HOST_ROOT_HUB_NDP);

	// initialize static buffers
	ohci_buffers_initialize();

	ohci_initialize(&_hc);
}

static int _start_pipe(usb_host_controller_t *hcnt, usb_host_pipe_t *pipe)
{
	if (pipe == NULL || pipe->Device == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (pipe->Endpoint != NULL)
		kernel_panic(KERNEL_ERROR_ALREADY_IN_USE);
	
	OHCI_SED *sed = ohci_buffers_alloc_sed();
	if (sed != NULL)
	{
		OHCI_HCED *hced = &sed->HCED;
		ohci_clear_hced(hced);
		hced->ControlBits.FunctionAddress = pipe->Device->Address;
		hced->ControlBits.Endpoint = pipe->EndpointNumber;
		hced->ControlBits.MaxPacketSize = pipe->MaxPacketSize;
		hced->ControlBits.Speed = (pipe->Device->Speed == USB_HOST_DEVICE_LOW_SPEED) ? 1 : 0;
		hced->ControlBits.Format = (pipe->EndpointType == USB_TT_ISO) ? 1 : 0;
		hced->ControlBits.Direction = (pipe->EndpointType == USB_TT_CONTROL) ? OHCI_ED_DIR_FROM_TD :
			(pipe->Direction == USB_HOST_TO_DEVICE ? OHCI_ED_DIR_OUT : OHCI_ED_DIR_IN);

		OHCI_STD *std = ohci_buffers_alloc_std();
		if (std != NULL) 
		{
			OHCI_HCTD *hctd = &std->HCTD;
			ohci_clear_hctd(hctd);
			hced->HeadTD = hctd;
			hced->TailTD = hctd;

			pipe->Endpoint = sed;
			sed->Pipe = pipe;

            ohci_pipe_add(pipe);
			return 1;
		}

		ohci_buffers_release_sed(sed);
	}
	return 0;
}

static int _stop_pipe(usb_host_controller_t *hcnt, usb_host_pipe_t *pipe)
{
	if (pipe == NULL || pipe->Device == NULL || pipe->Endpoint == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
    OHCI_SED *sed = pipe->Endpoint;
	if (sed->Pipe != pipe)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);	// sed or pipe is corrupted 

	ohci_pipe_remove(pipe);

	ohci_pipe_flush(pipe, NULL);

	OHCI_HCED *hced = &sed->HCED;
	OHCI_HCTD *hctd = (OHCI_HCTD *)((unsigned long)hced->HeadTD & ~0xF);
#ifdef DEBUG
	if (hctd != hced->TailTD)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);	// queue was not empty?
#endif
	ohci_buffers_release_std((OHCI_STD *)hctd);
	ohci_buffers_release_sed(sed);
	return 1;
}

static bool _begin_bulk_transfer(usb_host_controller_t *hcnt, usb_request_buffer_t *urb, void *data, unsigned length)
{
	// TODO: Add support for more than a single transfer per request

	OHCI_TD_PID pid = urb->Pipe->Direction == USB_HOST_TO_DEVICE ? OHCI_TD_DIR_OUT : OHCI_TD_DIR_IN;
    urb->Data = data;
	urb->Length = length;
	OHCI_STD *std = ohci_add_std(urb, NULL, pid, OHCI_TD_TOGGLE_CARRY);
	return (std != NULL);
}

static int _end_bulk_transfer(usb_host_controller_t *hcnt, usb_request_buffer_t *urb, unsigned timeout)
{
	if (urb == NULL || urb->Pipe == NULL || urb->UserState == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	OHCI_STD *std = (OHCI_STD *)urb->UserState;
#ifdef DEBUG
	if (std->Request != urb)
		kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);
#endif
	if (exos_event_wait(&urb->Event, timeout) == -1)
	{
		int removed = ohci_remove_std(urb);
		if (removed != 0) 
			return -1;
	}

	ohci_buffers_release_std(std);
	return (urb->Status == URB_STATUS_DONE) ? urb->Done : -1;
}

static bool _create_device(usb_host_controller_t *hc, usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed)
{
	static usb_device_descriptor_t _dev_desc __usb;	// NOTE: buffer for descriptors, not re-entrant (lock?)
	static volatile int _last_device = 0;	// FIXME

	bool done = false;
	exos_mutex_lock(&_mutex);

	if (_start_pipe(hc, &device->ControlPipe))
	{
		device->State = USB_HOST_DEVICE_ATTACHED;

		usb_host_pipe_t *pipe = &device->ControlPipe;
		usb_device_descriptor_t *dev_desc = &_dev_desc;

		// read device descriptor (header)
		done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_DEVICE, 0, dev_desc, 8);
		if (done)
		{
			OHCI_SED *sed = (OHCI_SED *)pipe->Endpoint;
			sed->HCED.ControlBits.MaxPacketSize = dev_desc->MaxPacketSize;
	
			// set address
			device->Address = ++_last_device;	// FIXME: search lowest unused
			done = usb_host_set_address(device, device->Address);
			if (done)
			{
				sed->HCED.ControlBits.FunctionAddress = device->Address;

				// read device descriptor (complete) 
				done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_DEVICE, 0, 
					dev_desc, sizeof(usb_device_descriptor_t));
		
				if (done)
				{
					done = usb_host_enumerate(device, dev_desc); 
				}
			}
		}

		if (!done)
		{
			_destroy_device(hc, device);
		}
	}

	exos_mutex_unlock(&_mutex);
	return done;
}

static void _destroy_device(usb_host_controller_t *hc, usb_host_device_t *device)
{
	exos_mutex_lock(&_mutex);

	if (device->State != USB_HOST_DEVICE_DETACHED)
	{
		device->State = USB_HOST_DEVICE_DETACHED;
		_stop_pipe(hc, &device->ControlPipe);
	}

	exos_mutex_unlock(&_mutex);
}

static bool _ctrl_setup_read(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *in_data, unsigned in_length)
{
	usb_request_buffer_t urb;

	exos_mutex_lock(&_mutex);

	usb_host_pipe_t *pipe = &device->ControlPipe;
	usb_host_urb_create(&urb, pipe);
	bool done = ohci_process_std(&urb, OHCI_TD_SETUP, OHCI_TD_TOGGLE_0, setup_data, setup_length);
	if (done) 
	{
		if (in_length) 
		{
			done = ohci_process_std(&urb, OHCI_TD_DIR_IN, OHCI_TD_TOGGLE_1, in_data, in_length);
		}
		if (done) 
		{
			done = ohci_process_std(&urb, OHCI_TD_DIR_OUT, OHCI_TD_TOGGLE_1, NULL, 0);
		}
	}

	exos_mutex_unlock(&_mutex);
	return done;
}

static bool _ctrl_setup_write(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *out_data, unsigned out_length)
{
	usb_request_buffer_t urb;

	exos_mutex_lock(&_mutex);

	usb_host_pipe_t *pipe = &device->ControlPipe;
	usb_host_urb_create(&urb, pipe);
	bool done = ohci_process_std(&urb, OHCI_TD_SETUP, OHCI_TD_TOGGLE_0, setup_data, setup_length);
    if (done) 
	{
        if (out_length) 
		{
            done = ohci_process_std(&urb, OHCI_TD_DIR_OUT, OHCI_TD_TOGGLE_1, out_data, out_length);
        }
        if (done) 
		{
            done = ohci_process_std(&urb, OHCI_TD_DIR_IN, OHCI_TD_TOGGLE_1, NULL, 0);
        }
    }

	exos_mutex_unlock(&_mutex);
	return done;
}




