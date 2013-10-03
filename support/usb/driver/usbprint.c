#include "usbprint.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc);
static void _start(USB_HOST_FUNCTION *func);
static void _stop(USB_HOST_FUNCTION *func);

static const USB_HOST_FUNCTION_DRIVER _driver = { _check_interface, _start, _stop };
static USB_HOST_FUNCTION_DRIVER_NODE _driver_node;

static int _open(COMM_IO_ENTRY *io);
static void _close(COMM_IO_ENTRY *io);
static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length);

static const COMM_DRIVER _comm_driver = {
	.Open = _open, .Close = _close,
    .GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };
static COMM_DEVICE _comm_device = { .Driver = &_comm_driver, .PortCount = 1 };
static USBPRINT_FUNCTION _func __usb; // FIXME: support multiple instances

void usbprint_initialize()
{
	_func.State = USBPRINT_NOT_ATTACHED;

	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static int _protocol_matches(USB_INTERFACE_DESCRIPTOR *if_desc, USBPRINT_PROTOCOL *pproto)
{
	USBPRINT_PROTOCOL proto = (USBPRINT_PROTOCOL)if_desc->Protocol;
	switch(proto)
	{
		case USBPRINT_PROTOCOL_UNIDIRECTIONAL:
		case USBPRINT_PROTOCOL_BIDIRECTIONAL:
//		case USBPRINT_PROTOCOL_1284_4: currently unsupported
			*pproto = proto;
			return 1;
	}
	return 0;
}

static int _device_if_matches(USB_HOST_DEVICE *device, USB_INTERFACE_DESCRIPTOR *if_desc, USBPRINT_PROTOCOL *pproto)
{
        int val = (device->Vendor == 0x1cbe && device->Product == 0x0002);
	return val;
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		USBPRINT_PROTOCOL proto;
		if ((if_desc->InterfaceClass == USB_CLASS_PRINTER
			&& if_desc->InterfaceSubClass == USBPRINT_SUBCLASS_PRINTER
			&& _protocol_matches(if_desc, &proto)) ||
			_device_if_matches(device, if_desc,&proto))
		{
			USBPRINT_FUNCTION *func = &_func;
			if (func->State == USBPRINT_NOT_ATTACHED)
			{
				usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);
	
				USB_ENDPOINT_DESCRIPTOR *ep_desc;			
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
				return (USB_HOST_FUNCTION *)func;
			}
		}
	}
	return NULL;
}

static void _start(USB_HOST_FUNCTION *usb_func)
{
	USBPRINT_FUNCTION *func = (USBPRINT_FUNCTION *)usb_func;

	usb_host_start_pipe(&func->BulkOutputPipe);
	if (func->Protocol == USBPRINT_PROTOCOL_BIDIRECTIONAL)
		usb_host_start_pipe(&func->BulkInputPipe);

	if (exos_tree_find_path(NULL, "dev/usbprint") == NULL)
	{
		func->KernelDevice = (EXOS_TREE_DEVICE) {
			.Name = "usbprint",	// FIXME: allow more instances
			.DeviceType = EXOS_TREE_DEVICE_COMM,
			.Device = &_comm_device,
			.Unit = 0 };	// FIXME: allow more instances
		
		exos_tree_add_device(&func->KernelDevice, "dev");
	}
	func->Entry = NULL;
	func->State = USBPRINT_CLOSED;
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	USBPRINT_FUNCTION *func = (USBPRINT_FUNCTION *)usb_func;

	usb_host_stop_pipe(&func->BulkOutputPipe);
   	if (func->Protocol == USBPRINT_PROTOCOL_BIDIRECTIONAL)
		usb_host_stop_pipe(&func->BulkInputPipe);

	COMM_IO_ENTRY *io = func->Entry;
	if (io != NULL) comm_io_close(io);

	func->State = USBPRINT_NOT_ATTACHED;
}

static int _open(COMM_IO_ENTRY *io)
{
	if (io->Port == 0)
	{
		USBPRINT_FUNCTION *func = &_func;
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

static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	return -1;
}

static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	return -1;
}

static void _close(COMM_IO_ENTRY *io)
{
	if (io->Port == 0)
	{
		USBPRINT_FUNCTION *func = &_func;
		if (func->Entry == io &&
			(func->State == USBPRINT_READY || func->State == USBPRINT_ERROR))
		{
			func->State = USBPRINT_CLOSED;
			func->Entry = NULL;
			exos_event_set(&io->InputEvent);
		}
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	// TODO: implement bidirectional feature
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	if (io->Port == 0)
	{
		USBPRINT_FUNCTION *func = &_func;
		if (io == func->Entry)
		{
			int offset = 0;
			do
			{
				int part = length - offset;
				if (part > USBPRINT_BUFFER_SIZE) part = USBPRINT_BUFFER_SIZE;
				__mem_copy(func->Buffer, func->Buffer + part, buffer + offset);
				int done = usb_host_bulk_transfer(&func->BulkOutputPipe, func->Buffer, part, EXOS_TIMEOUT_NEVER);
				if (!done) return -1;
				offset += part;
			} while(offset < length);
			return offset;
		}
	}
	return -1;
}
