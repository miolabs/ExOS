#include "usbhub.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>
#include <kernel/dispatch.h>
#include <kernel/fifo.h>
#include <kernel/panic.h>
#include <support/services/debug.h>

#ifndef USB_HUB_MAX_INSTANCES 
#define USB_HUB_MAX_INSTANCES 1
#endif

#ifndef USB_HUB_MAX_CHILDREN 
#define USB_HUB_MAX_CHILDREN 4
#endif

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static EXOS_DISPATCHER_CONTEXT _service_context;
static void _dispatch(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
static void _start(USB_HOST_FUNCTION *func);
static void _stop(USB_HOST_FUNCTION *func);

static const USB_HOST_FUNCTION_DRIVER _driver = { _check_interface, _start, _stop };
static USB_HOST_FUNCTION_DRIVER_NODE _driver_node;

static USB_HUB_FUNCTION _functions[USB_HUB_MAX_INSTANCES] __usb;
static volatile unsigned char _function_busy[USB_HUB_MAX_INSTANCES];
static USB_HOST_DEVICE _children[USB_HUB_MAX_CHILDREN] __usb;
static EXOS_FIFO _children_fifo;

static int _read_hub_descriptor(USB_HUB_FUNCTION *func, int desc_type, int desc_index, void *data, int length);
static int _set_port_feature(USB_HUB_FUNCTION *func, unsigned short feature, unsigned short index);

void usbd_hub_initialize()
{
	for(int port = 0; port < USB_HUB_MAX_INSTANCES; port++)
	{
		_function_busy[port] = 0;
		USB_HUB_FUNCTION *func = &_functions[port];
		*func = (USB_HUB_FUNCTION) { /*.DeviceUnit = port*/ }; 
	}

	exos_fifo_create(&_children_fifo, NULL);
	for(int child = 0; child < USB_HUB_MAX_CHILDREN; child++)
	{
		USB_HOST_DEVICE *device = &_children[child];
		exos_fifo_queue(&_children_fifo, &device->Node);
	}

	exos_dispatcher_context_create(&_service_context);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, NULL, _service, &_service_context);

	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_HUB)
		{
			USB_HUB_FUNCTION *func = NULL;
			for(int i = 0; i < USB_HUB_MAX_INSTANCES; i++)
			{
				if (_function_busy[i] == 0)
				{
					func = &_functions[i];
					func->InstanceIndex = i;
				}
			}

			if (func != NULL)
			{
				usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);
				func->Interface = if_desc->InterfaceNumber;
//				func->InterfaceSubClass = if_desc->InterfaceSubClass;
//				func->Protocol = if_desc->Protocol;

				USB_ENDPOINT_DESCRIPTOR *ep_desc;			
				ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
				if (!ep_desc) return NULL;
				usb_host_init_pipe_from_descriptor(device, &func->InputPipe, ep_desc);

				int done = _read_hub_descriptor(func, USB_HUB_DESCRIPTOR_HUB, 0, 
					func->InputBuffer, sizeof(USB_HUB_DESCRIPTOR));
				if (done)
				{
					USB_HUB_DESCRIPTOR *hub_desc = (USB_HUB_DESCRIPTOR *)func->InputBuffer;
					func->PortCount = hub_desc->NbrPorts;
					func->ReportSize = (func->PortCount + 1 + 7) >> 3;
					list_initialize(&func->Children);
				
					return (USB_HOST_FUNCTION *)func;
				}
			}
		}
	}
	return NULL;
}

static void _start(USB_HOST_FUNCTION *usb_func)
{
	USB_HUB_FUNCTION *func = (USB_HUB_FUNCTION *)usb_func;
#ifdef DEBUG
	if (_function_busy[func->InstanceIndex] != 0)
		kernel_panic(KERNEL_ERROR_ALREADY_IN_USE);
#endif

	_function_busy[func->InstanceIndex] = 1;

	for(int port = 1; port <= func->PortCount; port++)
	{
		_set_port_feature(func, USB_HUB_FEATURE_PORT_POWER, port);
	}

	usb_host_start_pipe(&func->InputPipe);

	exos_event_create(&func->ExitEvent);
	func->StartedFlag = func->ExitFlag = 0;
	func->Dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch, .CallbackState = func };
	exos_dispatcher_add(&_service_context, &func->Dispatcher, 0);
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	USB_HUB_FUNCTION *func = (USB_HUB_FUNCTION *)usb_func;

	func->ExitFlag = 1;
	usb_host_stop_pipe(&func->InputPipe);

	exos_event_wait(&func->ExitEvent, EXOS_TIMEOUT_NEVER);
	_function_busy[func->InstanceIndex] = 0;
}

static int _read_hub_descriptor(USB_HUB_FUNCTION *func, int desc_type, int desc_index, void *data, int length)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Length = length };
	return usb_host_ctrl_setup(func->Device, &req, data, length);
}

static int _clear_hub_feature(USB_HUB_FUNCTION *func, unsigned short feature)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_CLEAR_FEATURE,
		.Value = feature };
	int done = usb_host_ctrl_setup(func->Device, &req, NULL, 0);
	return done;
}

static int _set_hub_feature(USB_HUB_FUNCTION *func, unsigned short feature)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_FEATURE,
		.Value = feature };
	int done = usb_host_ctrl_setup(func->Device, &req, NULL, 0);
	return done;
}

static int _clear_port_feature(USB_HUB_FUNCTION *func, unsigned short feature, unsigned short index)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_CLEAR_FEATURE,
		.Value = feature, .Index = index };
	int done = usb_host_ctrl_setup(func->Device, &req, NULL, 0);
	return done;
}

static int _set_port_feature(USB_HUB_FUNCTION *func, unsigned short feature, unsigned short index)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_SET_FEATURE,
		.Value = feature, .Index = index };
	int done = usb_host_ctrl_setup(func->Device, &req, NULL, 0);
	return done;
}

static int _get_port_status(USB_HUB_FUNCTION *func, int port)
{
	USB_REQUEST req = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_GET_STATUS,
		.Index = port, .Length = 4 };
	int done = usb_host_ctrl_setup(func->Device, &req, func->InputBuffer, 4);
	return done;
}

static void *_service(void *arg)
{
	EXOS_DISPATCHER_CONTEXT *context = (EXOS_DISPATCHER_CONTEXT *)arg;
	while(1)
	{
		exos_dispatch(&_service_context, EXOS_TIMEOUT_NEVER);
	}
}

static USB_HOST_DEVICE *_alloc_device()
{
	return (USB_HOST_DEVICE *)exos_fifo_dequeue(&_children_fifo);
}

static void _free_device(USB_HOST_DEVICE *child)
{
	debug_printf("usb_hub: child %04x/%04x removing at port #%d\r\n", child->Vendor, child->Product, child->Port);
	usb_host_destroy_child_device(child);
	debug_printf("usb_hub: child %04x/%04x removed\r\n", child->Vendor, child->Product);
	exos_fifo_queue(&_children_fifo, &child->Node);
}

static void _notify(USB_HUB_FUNCTION *func, int port)
{
	_get_port_status(func, port);
	USB16 *nw = (USB16 *)func->InputBuffer;
	unsigned short port_status = USB16TOH(nw[0]);
	unsigned short port_change = USB16TOH(nw[1]);

	if (port_change & USB_HUBF_PORT_CONNECTION)
	{
		if (port_status & USB_HUBF_PORT_CONNECTION)
		{
			exos_thread_sleep(50);	// at least 50ms before reset as of USB 2.0 spec
			_set_port_feature(func, USB_HUB_FEATURE_PORT_RESET, port);
		}
		else
		{
			FOREACH(node, &func->Children)
			{
				USB_HOST_DEVICE *child = (USB_HOST_DEVICE *)node;
				if (child->Port == port)
				{
					list_remove((EXOS_NODE *)child);
					_free_device(child);
					break;
				}
			}
		}

		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_CONNECTION, port);
	}

	if (port_change & USB_HUBF_PORT_RESET)
	{
		if (port_status & USB_HUBF_PORT_CONNECTION)
		{
			exos_thread_sleep(100);	// some devices need up to 100ms after port reset

			USB_HOST_DEVICE *child = _alloc_device();
			if (child != NULL)
			{
				USB_HOST_DEVICE_SPEED speed = (port_status & USB_HUBF_PORT_LOW_SPEED) ?
					USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
				int done = usb_host_create_child_device(func->Device, child, port, speed);
				if (done)
				{
					debug_printf("usb_hub: child %04x/%04x added at port #%d\r\n", child->Vendor, child->Product, child->Port);
					list_add_tail(&func->Children, &child->Node);
				}
			}
		}

		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_RESET, port);
	}

	if (port_change & USB_HUBF_PORT_ENABLE)
	{
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_ENABLE, port);
	}

	if (port_change & USB_HUBF_PORT_OVER_CURRENT)
	{
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_OVER_CURRENT, port);
	}
}

static void _dispatch(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	USB_HUB_FUNCTION *func = (USB_HUB_FUNCTION *)dispatcher->CallbackState;
#ifdef DEBUG
	if (func == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (func->StartedFlag)
	{
		int done = usb_host_end_bulk_transfer(&func->Urb, EXOS_TIMEOUT_NEVER);
		if (done >= 0)
		{
			for (int port = 1; port <= func->PortCount; port++)
			{
				if (func->InputBuffer[port >> 3] & (1 << (port & 7)))
					_notify(func, port);
			}			
		}
		else 
		{
			// TODO: recover from failure state
			func->ExitFlag = 2;
		}
	}

	if (!func->ExitFlag)
	{
		usb_host_urb_create(&func->Urb, &func->InputPipe);
		if (usb_host_begin_bulk_transfer(&func->Urb, func->InputBuffer, func->ReportSize))
		{
			func->StartedFlag = 1;
			dispatcher->Event = &func->Urb.Event;
            exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
		}
        else func->ExitFlag = 3;
	}

	if (func->ExitFlag)
	{
		USB_HOST_DEVICE *child = NULL;
		while(child = (USB_HOST_DEVICE *)list_rem_head(&func->Children))
		{
			_free_device(child);
		}
		exos_event_set(&func->ExitEvent);
	}
}


