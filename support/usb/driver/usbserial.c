#include "usbserial.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>
#include <kernel/dispatch.h>
#include <kernel/thread.h>
#include <kernel/panic.h>

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
static USBSERIAL_FUNCTION _func __usb; // FIXME: support multiple instances

static unsigned short _vendor_id = 0x0519;
static unsigned short _product_id = 0x1001;

void usbserial_initialize(unsigned short vendor_id, unsigned short product_id)
{        
        _vendor_id = vendor_id;
        _product_id = product_id;
	_func.State = USBSERIAL_NOT_ATTACHED;

	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static int _device_if_matches(USB_HOST_DEVICE *device, USB_INTERFACE_DESCRIPTOR *if_desc)
{
	return device->Vendor == _vendor_id
		&& device->Product == _product_id;
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{

        USB_ENDPOINT_DESCRIPTOR *in_desc;
	USB_ENDPOINT_DESCRIPTOR *out_desc;
	USB_ENDPOINT_DESCRIPTOR *int_desc;
	
        if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;
              
		if (if_desc->InterfaceClass == USB_CLASS_COMM
			&& if_desc->InterfaceSubClass == USB_CDC_SUBCLASS_ACM
			&& _device_if_matches(device, if_desc))
		{
			USBSERIAL_FUNCTION *func = &_func;
			if (func->State == USBSERIAL_NOT_ATTACHED)
			{
				usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);

                                out_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
                                in_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
                                int_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
                                if (!in_desc || !out_desc) return NULL;

                                usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, out_desc);
                                usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, in_desc);
                                if (int_desc != NULL) usb_host_init_pipe_from_descriptor(device, &func->InterruptPipe, int_desc);

                                 func->Device = device;

                                func->Interface = if_desc->InterfaceNumber;
//					func->Protocol = proto;
                                func->State = USBSERIAL_ATTACHING;
                                return (USB_HOST_FUNCTION *)func;
			}
		}
	}
	return NULL;
}

#define ACM_CTRL_DTR 0x01
#define ACM_CTRL_RTS 0x02

static void _start(USB_HOST_FUNCTION *usb_func)
{
	USBSERIAL_FUNCTION *func = (USBSERIAL_FUNCTION *)usb_func;

         USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = 0x21,
		.RequestCode = 0x22,
		.Value = ACM_CTRL_DTR | ACM_CTRL_RTS, 
                .Index = 0, 
                .Length = 0 };
                
        int done = usb_host_ctrl_setup(func->Device, &setup, NULL, 0);

	usb_host_start_pipe(&func->BulkOutputPipe);
	usb_host_start_pipe(&func->BulkInputPipe);
        usb_host_start_pipe(&func->InterruptPipe);

	if (exos_tree_find_path(NULL, "dev/usbserial") == NULL)
	{
		func->KernelDevice = (EXOS_TREE_DEVICE) {
			.Name = "usbserial",	// FIXME: allow more instances
			.Device = &_comm_device,
			.Unit = 0 };	// FIXME: allow more instances
		
		comm_add_device(&func->KernelDevice, "dev");
	}
	func->Entry = NULL;
	func->State = USBSERIAL_CLOSED;
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	USBSERIAL_FUNCTION *func = (USBSERIAL_FUNCTION *)usb_func;

	usb_host_stop_pipe(&func->BulkOutputPipe);
   	usb_host_stop_pipe(&func->BulkInputPipe);
        usb_host_stop_pipe(&func->InterruptPipe);

	COMM_IO_ENTRY *io = func->Entry;
	if (io != NULL) comm_io_close(io);

	func->State = USBSERIAL_NOT_ATTACHED;
}

static int _open(COMM_IO_ENTRY *io)
{
	USBSERIAL_FUNCTION *func = &_func;
	if (func->State == USBSERIAL_CLOSED && func->Entry == NULL)
	{		              
		func->State = USBSERIAL_READY;
		func->Entry = io;

		exos_event_set(&io->OutputEvent);
                exos_event_set(&io->InputEvent);
		return 0;
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
		USBSERIAL_FUNCTION *func = &_func;
		if (func->Entry == io &&
			(func->State == USBSERIAL_READY || func->State == USBSERIAL_ERROR))
		{
			func->State = USBSERIAL_CLOSED;
			func->Entry = NULL;
			exos_event_set(&io->InputEvent);
		}
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	// TODO: implement bidirectional feature
        if (io->Port == 0)
	{
		USBSERIAL_FUNCTION *func = &_func;
		if (io == func->Entry)
		{
                        int done = usb_host_bulk_transfer(&func->BulkInputPipe, buffer, length, EXOS_TIMEOUT_NEVER);
			return done;
		}
	}
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	if (io->Port == 0)
	{
		USBSERIAL_FUNCTION *func = &_func;
		if (io == func->Entry)
		{
			int offset = 0;
			do
			{
				int part = length - offset;
				if (part > USBSERIAL_USB_BUFFER) part = USBSERIAL_USB_BUFFER;
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

