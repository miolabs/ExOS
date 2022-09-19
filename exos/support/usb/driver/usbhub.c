#include "usbhub.h"
#include <usb/enumerate.h>
#include <support/misc/pools.h>
#include <kernel/panic.h>
#include <kernel/verbose.h>

#ifdef USB_HUB_DEBUG
#define _verbose(level, ...) verbose(level, "usb-hub", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

#ifndef USB_HUB_MAX_INSTANCES 
#define USB_HUB_MAX_INSTANCES 1
#endif

#ifndef USB_HUB_MAX_CHILDREN
#define USB_HUB_MAX_CHILDREN 4
#endif

static usb_host_function_t *_check_interface(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
static void _start(usb_host_function_t *func, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
static void _stop(usb_host_function_t *func);
static const usb_host_function_driver_t _driver = { 
	.CheckInterface = _check_interface, 
	.Start =_start, .Stop = _stop };

static usb_hub_function_t _functions[USB_HUB_MAX_INSTANCES] __usb;
static volatile unsigned char _function_busy[USB_HUB_MAX_INSTANCES];
static usb_host_device_t _children[USB_HUB_MAX_CHILDREN];
static pool_t _pool;
static dispatcher_context_t *_context;

static bool _read_hub_descriptor(usb_hub_function_t *func, unsigned char desc_type, unsigned char desc_index, void *data, unsigned length);
static bool _get_hub_status(usb_hub_function_t *func);
static bool _set_port_feature(usb_hub_function_t *func, unsigned short feature, unsigned short index);
static bool _get_port_status(usb_hub_function_t *func, unsigned short port);
static void _dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher);

void usb_hub_initialize(dispatcher_context_t *context)
{
	for(unsigned port = 0; port < USB_HUB_MAX_INSTANCES; port++)
	{
		_function_busy[port] = 0;
		usb_hub_function_t *func = &_functions[port];
		*func = (usb_hub_function_t) { /*.DeviceUnit = port*/ }; 
	}

	pool_create(&_pool, (node_t *)_children, sizeof(usb_host_device_t), USB_HUB_MAX_CHILDREN);

	_context = context;

	static usb_host_function_driver_node_t _driver_node = (usb_host_function_driver_node_t) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static usb_host_function_t *_check_interface(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		usb_interface_descriptor_t *if_desc = (usb_interface_descriptor_t *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_HUB)
		{
			usb_hub_function_t *func = nullptr;
			for(int i = 0; i < USB_HUB_MAX_INSTANCES; i++)
			{
				if (_function_busy[i] == 0)
				{
					func = &_functions[i];
					func->InstanceIndex = i;
				}
			}

			if (func != nullptr)
			{
				_verbose(VERBOSE_COMMENT, "hub if_descriptor indicates protocol %d", if_desc->Protocol);

				usb_host_create_function((usb_host_function_t *)func, device, &_driver);
				func->Interface = if_desc->InterfaceNumber;

				usb_endpoint_descriptor_t *ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, 
					USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
				if (ep_desc != nullptr)
				{
					usb_host_init_pipe_from_descriptor(device, &func->InputPipe, ep_desc);
					
					if (_read_hub_descriptor(func, USB_HUB_DESCRIPTOR_HUB, 0, 
							func->InputBuffer, sizeof(usb_hub_descriptor_t)))
					{
						usb_hub_descriptor_t *hub_desc = (usb_hub_descriptor_t *)func->InputBuffer;
						func->PortCount = hub_desc->NbrPorts;
						func->ReportSize = (func->PortCount + 1 + 7) >> 3;
						list_initialize(&func->Children);

						func->PwrOn2PwrGood = hub_desc->PwrOn2PwrGood;
						func->HubCharacteristics = USB16TOH(hub_desc->HubCharacteristics);
				
						return (usb_host_function_t *)func;
					}
				}
			}
		}
	}
	return nullptr;
}

static void _start(usb_host_function_t *usb_func, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc)
{
	usb_hub_function_t *func = (usb_hub_function_t *)usb_func;
	ASSERT(_function_busy[func->InstanceIndex] == 0, KERNEL_ERROR_KERNEL_PANIC);
	_function_busy[func->InstanceIndex] = 1;

	bool done = _get_hub_status(func);	// NOTE: currently return data is unused
	ASSERT(done, KERNEL_ERROR_KERNEL_PANIC);

	_verbose(VERBOSE_COMMENT, "hub logical power control is %s",
		((func->HubCharacteristics & USB_HUB_CHARF_LOGICAL_POWER_MASK) == USB_HUB_CHARF_LOGICAL_POWER_INDIVIDUAL) ? "individual" : "global");
	_verbose(VERBOSE_COMMENT, "hub over-current protection mode is %s",
		((func->HubCharacteristics & USB_HUB_CHARF_OVERCURRENT_MODE_MASK) == USB_HUB_CHARF_OVERCURRENT_MODE_INDIVIDUAL) ? "individual" : "global");
	_verbose(VERBOSE_COMMENT, "hub port indication is %s",
		(func->HubCharacteristics & USB_HUB_CHARF_PORT_INDICATORS_SUPPORTED) ? "supported" : "not supported");
	_verbose(VERBOSE_COMMENT, "hub port power delay is %d ms", func->PwrOn2PwrGood * 2);

	for(unsigned port = 1; port <= func->PortCount; port++)
	{
		_set_port_feature(func, USB_HUB_FEATURE_PORT_POWER, port);
		_get_port_status(func, port);
	}

	done = usb_host_start_pipe(&func->InputPipe);
	ASSERT(done, KERNEL_ERROR_KERNEL_PANIC);

	func->ExitFlag = 0;

	usb_host_urb_create(&func->Request, &func->InputPipe);
	exos_dispatcher_create(&func->Dispatcher, NULL, _dispatch, func);
	exos_dispatcher_add(_context, &func->Dispatcher, 0);

	_verbose(VERBOSE_COMMENT, "instance #%d started", func->InstanceIndex);
}

static void _free_device(usb_host_device_t *child);

static void _stop(usb_host_function_t *usb_func)
{
	usb_hub_function_t *func = (usb_hub_function_t *)usb_func;

	func->ExitFlag = 1;
	usb_host_stop_pipe(&func->InputPipe);

	exos_dispatcher_remove(_context, &func->Dispatcher);
	ASSERT(func->Request.Status != URB_STATUS_ISSUED, KERNEL_ERROR_KERNEL_PANIC);

	usb_host_device_t *child;
	while(child = (usb_host_device_t *)LIST_FIRST(&func->Children))
	{
		list_remove(&child->Node);
		_free_device(child);
	}

	_function_busy[func->InstanceIndex] = 0;
}

static bool _read_hub_descriptor(usb_hub_function_t *func, unsigned char desc_type, unsigned char desc_index, void *data, unsigned length)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_DESCRIPTOR,
		.Value = (desc_type << 8) | desc_index, .Length = length };
	return usb_host_ctrl_setup(func->Device, &req, data, length);
}

static bool _get_hub_status(usb_hub_function_t *func)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_GET_STATUS,
		.Length = 4 };
	bool done = usb_host_ctrl_setup(func->Device, &req, func->InputBuffer, 4);
#ifdef DEBUG
	if (!done) _verbose(VERBOSE_ERROR, "get_hub_status() failed");
#endif
	return done;
}

static bool _clear_hub_feature(usb_hub_function_t *func, unsigned short feature)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_CLEAR_FEATURE,
		.Value = feature };
	bool done = usb_host_ctrl_setup(func->Device, &req, nullptr, 0);
	return done;
}

static bool _set_hub_feature(usb_hub_function_t *func, unsigned short feature)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_REQUEST_SET_FEATURE,
		.Value = feature };
	bool done = usb_host_ctrl_setup(func->Device, &req, nullptr, 0);
	return done;
}

static bool _clear_port_feature(usb_hub_function_t *func, unsigned short feature, unsigned short index)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_CLEAR_FEATURE,
		.Value = feature, .Index = index };
	bool done = usb_host_ctrl_setup(func->Device, &req, nullptr, 0);
	return done;
}

static bool _set_port_feature(usb_hub_function_t *func, unsigned short feature, unsigned short index)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_SET_FEATURE,
		.Value = feature, .Index = index };
	bool done = usb_host_ctrl_setup(func->Device, &req, nullptr, 0);
	return done;
}

static bool _get_port_status(usb_hub_function_t *func, unsigned short port)
{
	usb_request_t req = (usb_request_t) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_CLASS | USB_REQTYPE_RECIPIENT_OTHER,
		.RequestCode = USB_REQUEST_GET_STATUS,
		.Index = port, .Length = 4 };
	bool done = usb_host_ctrl_setup(func->Device, &req, func->InputBuffer, 4);
#ifdef DEBUG
	if (!done) _verbose(VERBOSE_ERROR, "get_port_status(%d) failed", port);
#endif
	return done;
}

static usb_host_device_t *_alloc_device(usb_host_device_t *device, unsigned port, usb_host_device_speed_t speed)
{
	usb_host_device_t *child = (usb_host_device_t *)pool_allocate(&_pool);
	if (child != nullptr)
	{
		bool done = usb_host_create_child_device(device, child, port, speed);
		if (!done)
		{
			pool_free(&_pool, &child->Node);
			child = nullptr;
		}
	}
	else
	{
		_verbose(VERBOSE_ERROR, "can't allocate device por port #%d!", port);
	}
	return child;
}

static void _free_device(usb_host_device_t *child)
{
	_verbose(VERBOSE_COMMENT, "child %04x/%04x removing at port #%d", child->Vendor, child->Product, child->Port);
	usb_host_destroy_device(child);
	_verbose(VERBOSE_COMMENT, "child %04x/%04x removed", child->Vendor, child->Product);

	pool_free(&_pool, &child->Node);
}

static void _notify(usb_hub_function_t *func, unsigned port)
{
	static unsigned _reset_pending = 0;

	_get_port_status(func, port);
	usb16_t *nw = (usb16_t *)func->InputBuffer;
	unsigned short port_status = USB16TOH(nw[0]);
	unsigned short port_change = USB16TOH(nw[1]);
	usb_host_device_t *child;

//	_verbose(VERBOSE_DEBUG, "(pre) port #%d status %04x %04x", port,
//		USB16TOH(nw[0]), USB16TOH(nw[1]));

	if (port_change & USB_HUBF_PORT_CONNECTION)
	{
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_CONNECTION, port);

		if (port_status & USB_HUBF_PORT_CONNECTION)
		{
			exos_thread_sleep(100);	// at least 50ms before reset as of USB 2.0 spec
			if (_reset_pending == 0)
			{
				_verbose(VERBOSE_COMMENT, "connection at port #%d -> reset", port);
				_set_port_feature(func, USB_HUB_FEATURE_PORT_RESET, port);
			}
			else 
			{
				_verbose(VERBOSE_COMMENT, "connection at port #%d", port);
			}
			_reset_pending |= (1 << port);
		}
		else
		{
			_verbose(VERBOSE_COMMENT, "disconnection at port #%d", port);
			FOREACH(node, &func->Children)
			{
				child = (usb_host_device_t *)node;
				if (child->Port == port)
				{
					list_remove(&child->Node);
					_free_device(child);
					break;
				}
			}
		}
	}

	if (port_change & USB_HUBF_PORT_RESET)
	{
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_RESET, port);
		_verbose(VERBOSE_COMMENT, "reset completed at port #%d", port);
		_reset_pending &= ~(1 << port);

		_get_port_status(func, port);
        port_status = USB16TOH(nw[0]);

		if (port_status & USB_HUBF_PORT_CONNECTION)
		{
			usb_host_device_speed_t speed = (port_status & USB_HUBF_PORT_LOW_SPEED) ?
				USB_HOST_DEVICE_LOW_SPEED : USB_HOST_DEVICE_FULL_SPEED;
			_verbose(VERBOSE_DEBUG, "child device is %s", 
				speed == USB_HOST_DEVICE_LOW_SPEED ? "low-speed" : "full-speed");

			exos_thread_sleep(100);	// some devices need up to 100ms after port reset
			
			child = _alloc_device(func->Device, port, speed);
			if (child != nullptr)
			{
				_verbose(VERBOSE_COMMENT, "child %04x/%04x added at port #%d", child->Vendor, child->Product, child->Port);
				list_add_tail(&func->Children, &child->Node);
			}
			else
			{
				_verbose(VERBOSE_ERROR, "child creation failed at port #%d", port);
			}
		}
		else
		{
			_verbose(VERBOSE_ERROR, "port #%d not connected after reset!", port);
		}

		if (_reset_pending != 0)
		{
			port = 0;
			while(1)
			{
				unsigned mask = (1 << port);
				ASSERT(mask != 0, KERNEL_ERROR_KERNEL_PANIC);
				if (mask & _reset_pending)
				{
					_verbose(VERBOSE_COMMENT, "pending reset at port #%d", port);
					_set_port_feature(func, USB_HUB_FEATURE_PORT_RESET, port);
					break;
				}
				port++;
			}
		}
	}

	if (port_change & USB_HUBF_PORT_ENABLE)
	{
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_ENABLE, port);
        
		_get_port_status(func, port);
        port_status = USB16TOH(nw[0]);
		_verbose(VERBOSE_COMMENT, "port #%d %s", port,
			(port_status & USB_HUBF_PORT_CONNECTION) ? "enabled" : "disabled");
			//             ^^^^^^^^^^^^^^^^^^^^^^^^^????
	}

	if (port_change & USB_HUBF_PORT_OVER_CURRENT)
	{
		_verbose(VERBOSE_ERROR, "port #%d overcurrent", port);
		_clear_port_feature(func, USB_HUB_FEATURE_C_PORT_OVER_CURRENT, port);
	}

	if (port_change & USB_HUBF_PORT_POWER)
	{
		_verbose(VERBOSE_ERROR, "port #%d power changed", port);
	}

#ifdef DEBUG
//	_get_hub_status(func);
//	_verbose(VERBOSE_DEBUG, "(post) hub status %04x %04x", USB16TOH(nw[0]), USB16TOH(nw[1]));
#endif
}

static void _dispatch(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	usb_hub_function_t *func = (usb_hub_function_t *)dispatcher->CallbackState;
	ASSERT(func != nullptr, KERNEL_ERROR_NULL_POINTER);

	usb_request_buffer_t *urb = &func->Request;
   	if (urb->Status != URB_STATUS_EMPTY)
	{
		int done = usb_host_end_transfer(urb, EXOS_TIMEOUT_NEVER);
		if (done > 0)
		{
			if (func->InputBuffer[0] & (1 << 0))
			{
				_verbose(VERBOSE_DEBUG, "hub change detected");
				// TODO
			}

			for (unsigned port = 1; port <= func->PortCount; port++)
			{
				if (func->InputBuffer[port >> 3] & (1 << (port & 7)))
					_notify(func, port);
			}			
		}
		else 
		{
			_verbose(VERBOSE_ERROR, "control in failed (instance #%d)", func->InstanceIndex);
			// TODO: recover from failure state
			func->ExitFlag = 2;
		}
	}

	if (!func->ExitFlag)
	{
		usb_host_urb_create(urb, &func->InputPipe);
		if (usb_host_begin_transfer(urb, func->InputBuffer, func->ReportSize))
		{
			dispatcher->Event = &urb->Event;
			exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
		}
        else func->ExitFlag = 3;
	}

	if (func->ExitFlag != 0)
	{
		_verbose(VERBOSE_DEBUG, "dispatcher exit code %d", func->ExitFlag);
	}
}


