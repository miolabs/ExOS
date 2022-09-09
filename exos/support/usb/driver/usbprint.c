#include "usbprint.h"
#include <usb/enumerate.h>
#include <kernel/io.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <string.h>

#ifdef DEBUG
#define _verbose(level, ...) verbose(level, "usbprint", __VA_ARGS__)
#else
#devine _verbose(level, ...) { /* nothing */ }
#endif

static usb_host_function_t *_check_interface(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc);
static void _start(usb_host_function_t *func);
static void _stop(usb_host_function_t *func);

static const usb_host_function_driver_t _driver = { _check_interface, _start, _stop };
static usb_host_function_driver_node_t _driver_node;

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
//static int _get_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value);
//static int _set_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	//.GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };

static usbprint_function_t _func __usb; // NOTE: single instance

static unsigned short _vendor_id = 0x1cbe;
static unsigned short _product_id = 0x0002;

void usbprint_initialize(unsigned short vendor_id, unsigned short product_id)
{
	_vendor_id = vendor_id;
	_product_id = product_id;
	_func.State = USBPRINT_NOT_ATTACHED;

	_driver_node = (usb_host_function_driver_node_t) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static bool _protocol_matches(usb_interface_descriptor_t *if_desc, usbprint_protocol_t *pproto)
{
	usbprint_protocol_t proto = (usbprint_protocol_t)if_desc->Protocol;
	switch(proto)
	{
		case USBPRINT_PROTOCOL_UNIDIRECTIONAL:
		case USBPRINT_PROTOCOL_BIDIRECTIONAL:
			*pproto = proto;
			return true;
		case USBPRINT_PROTOCOL_1284_4: //currently unsupported
		default:		
			_verbose(VERBOSE_ERROR, "protocol not supported");
			break;
	}
	return false;
}

static bool _device_if_matches(usb_host_device_t *device, usb_interface_descriptor_t *if_desc, usbprint_protocol_t *pproto)
{
	return device->Vendor == _vendor_id
		&& device->Product == _product_id;
}

static usb_host_function_t *_check_interface(usb_host_device_t *device, 
	usb_configuration_descriptor_t *conf_desc, usb_descriptor_header_t *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		usb_interface_descriptor_t *if_desc = (usb_interface_descriptor_t *)fn_desc;

		usbprint_protocol_t proto;
		if ((if_desc->InterfaceClass == USB_CLASS_PRINTER
			&& if_desc->InterfaceSubClass == USBPRINT_SUBCLASS_PRINTER
			&& _protocol_matches(if_desc, &proto)) ||
			_device_if_matches(device, if_desc, &proto))
		{
			usbprint_function_t *func = &_func;	// NOTE: single instance
			if (func->State == USBPRINT_NOT_ATTACHED)
			{
				usb_host_create_function((usb_host_function_t *)func, device, &_driver);
	
				usb_endpoint_descriptor_t *ep_desc;			
				ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
				if (!ep_desc) return NULL;
				usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, ep_desc);
	
				if (proto == USBPRINT_PROTOCOL_BIDIRECTIONAL)
				{
					ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
					if (!ep_desc) return NULL;
					usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, ep_desc);
				}
	
				func->Interface = if_desc->InterfaceNumber;
				func->Protocol = proto;
				func->State = USBPRINT_ATTACHING;
				return (usb_host_function_t *)func;
			}
		}
	}
	return NULL;
}

static void _start(usb_host_function_t *usb_func)
{
	usbprint_function_t *func = (usbprint_function_t *)usb_func;

	usb_host_start_pipe(&func->BulkOutputPipe);
	if (func->Protocol == USBPRINT_PROTOCOL_BIDIRECTIONAL)
		usb_host_start_pipe(&func->BulkInputPipe);

	if (exos_tree_find_path(NULL, "dev/usbprint") == NULL)
	{
		// NOTE: single instance
		exos_io_add_device(&func->KernelDevice, "usbprint", &_io_driver, func);
	}
	func->Entry = NULL;
	func->State = USBPRINT_CLOSED;
}

static void _stop(usb_host_function_t *usb_func)
{
	usbprint_function_t *func = (usbprint_function_t *)usb_func;

	usb_host_stop_pipe(&func->BulkOutputPipe);
   	if (func->Protocol == USBPRINT_PROTOCOL_BIDIRECTIONAL)
		usb_host_stop_pipe(&func->BulkInputPipe);

	io_entry_t *io = func->Entry;
	if (io != NULL) exos_io_close(io);

	func->State = USBPRINT_NOT_ATTACHED;
}

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	if (io->Port == 0)
	{
		usbprint_function_t *func = &_func;
		if (func->State == USBPRINT_CLOSED && func->Entry == NULL)
		{
			func->State = USBPRINT_READY;
			func->Entry = io;
			exos_event_set(&io->OutputEvent);
			return 0;
		}
	}
	return -1;
}

//static int _get_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value)
//{
//	return -1;
//}

//static int _set_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value)
//{
//	return -1;
//}

static void _close(io_entry_t *io)
{
	if (io->Port == 0)
	{
		usbprint_function_t *func = &_func;
		if (func->Entry == io &&
			(func->State == USBPRINT_READY || func->State == USBPRINT_ERROR))
		{
			func->State = USBPRINT_CLOSED;
			func->Entry = NULL;
			exos_event_set(&io->InputEvent);
		}
	}
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	// TODO: implement bidirectional feature
	return -1;
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	if (io->Port == 0)
	{
		usbprint_function_t *func = &_func;
		if (io == func->Entry)
		{
			int offset = 0;
			do
			{
				unsigned part = length - offset;
				if (part > USBPRINT_USB_BUFFER) part = USBPRINT_USB_BUFFER;
				memcpy(func->Buffer, buffer + offset, part);
				bool done = usb_host_do_transfer(&func->BulkOutputPipe, func->Buffer, part, EXOS_TIMEOUT_NEVER);
				if (!done) return -1;
				offset += part;
			} while(offset < length);
			return offset;
		}
	}
	return -1;
}
