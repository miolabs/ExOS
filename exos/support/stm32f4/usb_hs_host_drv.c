#include "usb_hs_host_drv.h"
#include "usb_hs_host_core.h"
#include "usb_otg_hs.h"
#include "cpu.h"
#include <usb/host.h>
#include <usb/enumerate.h>
#include <kernel/event.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>

#ifdef STM32_USB_DEBUG
#define _verbose(level, ...)	verbose(level, "usb-hs-host", __VA_ARGS__)
#else
#define _verbose(level, ...)	{ /* nothing */ }
#endif

static mutex_t _mutex;

static bool _ctrl_setup_read(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *in_data, unsigned in_length);
static bool _ctrl_setup_write(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *out_data, unsigned out_length);
static bool _start_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
static bool _stop_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe);
static bool _begin_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, void *data, unsigned length);
static int _end_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, unsigned timeout);
static bool _create_device(usb_host_controller_t *hc, usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed);
static void _destroy_device(usb_host_controller_t *hc, usb_host_device_t *device);

static const usb_host_controller_driver_t _driver = {
	_ctrl_setup_read, _ctrl_setup_write, 
	_start_pipe, _stop_pipe, _begin_transfer, _end_transfer,
	_create_device, _destroy_device };

static usb_host_controller_t _hc;
static usb_host_device_t _root_device;

void usb_hs_init_as_host(dispatcher_context_t *context)
{
	ASSERT(context != nullptr, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_create(&_mutex);
	usb_host_controller_create(&_hc, &_driver, &_root_device, 1);
	usb_hs_host_initialize(&_hc, context);
}

static bool _begin_xfer(usb_request_buffer_t *urb, usb_direction_t dir, bool setup, void *data, unsigned length)
{
	ASSERT(urb != nullptr && urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(urb->Status != URB_STATUS_ISSUED, KERNEL_ERROR_KERNEL_PANIC);
	urb->Data = data;
	urb->Length = length;
	urb->Done = 0;
	urb->Status = URB_STATUS_ISSUED;

	return usb_hs_host_begin_xfer(urb, dir, setup);
}

// HC driver
//---------------------

static bool _start_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr && pipe->Device != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pipe->Endpoint == nullptr, KERNEL_ERROR_KERNEL_PANIC);	// already in use

	return usb_hs_host_start_pipe(pipe);
}

static bool _stop_pipe(usb_host_controller_t *hc, usb_host_pipe_t *pipe)
{
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pipe->Device != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pipe->Endpoint != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	usb_hs_host_stop_pipe(pipe);
	return true;
}

static bool _begin_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, void *data, unsigned length)
{
	usb_host_pipe_t *pipe = urb->Pipe;
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	stm32_usbh_ep_t *ep = pipe->Endpoint;
	ASSERT(ep != nullptr, KERNEL_ERROR_NULL_POINTER);

	return _begin_xfer(urb, urb->Pipe->Direction, false, data, length);
}

static int _end_transfer(usb_host_controller_t *hc, usb_request_buffer_t *urb, unsigned timeout)
{
	ASSERT(urb != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(urb->Pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	return (urb->Status == URB_STATUS_DONE) ? urb->Done : -1;
}

static void _destroy_device(usb_host_controller_t *hc, usb_host_device_t *device)
{
	exos_mutex_lock(&_mutex);

	if (device->State != USB_HOST_DEVICE_DETACHED)
	{
		device->State = USB_HOST_DEVICE_DETACHED;
//		_stop_pipe(hc, &device->ControlPipe);	// FIXME
	}

	exos_mutex_unlock(&_mutex);
}

static bool _create_device(usb_host_controller_t *hc, usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed)
{
	static usb_device_descriptor_t _dev_desc __usb;	// NOTE: buffer for descriptors, not re-entrant
	static volatile unsigned char _last_device = 0;	// FIXME

	bool done = false;
	exos_mutex_lock(&_mutex);

	usb_host_pipe_t *control_pipe = &device->ControlPipe;
	if (usb_hs_host_start_pipe(control_pipe))
	{
		device->State = USB_HOST_DEVICE_ATTACHED;

		usb_device_descriptor_t *dev_desc = &_dev_desc;

		// read device descriptor (header)
		done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_DEVICE, 0, dev_desc, 8);
		if (done)
		{
			if (device->Speed != USB_HOST_DEVICE_LOW_SPEED)
			{
				// NOTE: full-speed devices can use more than (the minimum) 8 byte packet size
				control_pipe->MaxPacketSize = dev_desc->MaxPacketSize;
				usb_hs_host_update_control_pipe(control_pipe);
			}
			else
			{
				_verbose(VERBOSE_COMMENT, "device is LOW_SPEED");
			}

			// set address
			unsigned char addr = ++_last_device;	// FIXME: search lowest unused
			done = usb_host_set_address(device, addr);
			if (done)
			{
				device->Address = addr; 
				usb_hs_host_update_control_pipe(control_pipe);

				// read device descriptor (complete) 
				done = usb_host_read_device_descriptor(device, USB_DESCRIPTOR_TYPE_DEVICE, 0, 
					dev_desc, sizeof(usb_device_descriptor_t));
		
				if (done)
				{
					done = usb_host_enumerate(device, dev_desc); 
					if (!done)
						_verbose(VERBOSE_DEBUG, "device enumeration failed (port #%d)", device->Port);
				}
				else _verbose(VERBOSE_ERROR, "cannot read device descriptor (full)");
			}
#ifdef DEBUG
			else kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
#endif
		}
		else _verbose(VERBOSE_ERROR, "cannot read device descriptor (short)");

		if (!done)
		{
			_destroy_device(hc, device);
		}
	}

	exos_mutex_unlock(&_mutex);
	return done;
}

static bool _do_control_xfer(usb_request_buffer_t *urb, usb_direction_t dir, bool setup, void *data, unsigned length)
{
	ASSERT(urb != nullptr, KERNEL_ERROR_NULL_POINTER);
	usb_host_pipe_t *pipe = urb->Pipe;
	ASSERT(pipe != nullptr, KERNEL_ERROR_NULL_POINTER);
	stm32_usbh_ep_t *ep = (stm32_usbh_ep_t *)urb->Pipe->Endpoint;
	ASSERT(ep != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	if (setup)
	{
		// NOTE: redefine control channels according to device/pipe before starting control request
		usb_hs_host_update_control_pipe(pipe);
	}

	if (_begin_xfer(urb, dir, setup, data, length))
	{
#ifdef DEBUG
		while (!exos_event_wait(&urb->Event, 3000))
			_verbose(VERBOSE_ERROR, "still waiting...");
#else
		exos_event_wait(&urb->Event, TIMEOUT_NEVER);
#endif
		ASSERT(ep->Status == STM32_EP_STA_IDLE, KERNEL_ERROR_KERNEL_PANIC);
	}
	return (urb->Status == URB_STATUS_DONE);
}

static bool _ctrl_setup_read(usb_host_device_t *device, void *setup_data, unsigned setup_length, void *in_data, unsigned in_length)
{
	usb_request_buffer_t urb;

	exos_mutex_lock(&_mutex);

	usb_host_pipe_t *pipe = &device->ControlPipe;
	usb_host_urb_create(&urb, pipe);

	bool done = _do_control_xfer(&urb, USB_HOST_TO_DEVICE, true, setup_data, setup_length);
	if (done) 
	{
		if (in_length) 
		{
			done = _do_control_xfer(&urb, USB_DEVICE_TO_HOST, false, in_data, in_length);
		}
		if (done) 
		{
			done = _do_control_xfer(&urb, USB_HOST_TO_DEVICE, false, nullptr, 0);
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
	bool done = _do_control_xfer(&urb, USB_HOST_TO_DEVICE, true, setup_data, setup_length);
    if (done) 
	{
		if (out_length) 
		{
			// FIXME: some devices require to issue setup and data/status phase in different frames
			if (device->Speed == USB_HOST_DEVICE_LOW_SPEED)
				exos_thread_sleep(2);

			done = _do_control_xfer(&urb, USB_HOST_TO_DEVICE, false, out_data, out_length);
		}
		if (done) 
		{
            done = _do_control_xfer(&urb, USB_DEVICE_TO_HOST, false, nullptr, 0);
        }
    }

	exos_mutex_unlock(&_mutex);
	return done;
}



