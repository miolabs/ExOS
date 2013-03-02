#include "ftdi.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>

#ifndef FTDI_BAUDRATE
#define FTDI_BAUDRATE 9600
#endif

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
static FTDI_FUNCTION _func __usb; // FIXME: support multiple instances

void ftdi_initialize()
{
	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static int _device_if_matches(USB_HOST_DEVICE *device, USB_INTERFACE_DESCRIPTOR *if_desc)
{
	return device->Vendor == 0x0403 // FTDI
		&& device->Product == 0x6001; // FT232
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		if (if_desc->InterfaceClass == USB_CLASS_CUSTOM
			&& _device_if_matches(device, if_desc))
		{
			FTDI_FUNCTION *func = &_func;
			usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);

			USB_ENDPOINT_DESCRIPTOR *ep_desc;			
			ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
			if (!ep_desc) return NULL;
			usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, ep_desc);

			ep_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
			if (!ep_desc) return NULL;
			usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, ep_desc);

			func->Interface = if_desc->InterfaceNumber;
			func->Protocol = 0;
			return (USB_HOST_FUNCTION *)func;
		}
	}
	return NULL;
}

static int _setup(FTDI_FUNCTION *func, unsigned long baudrate, FTDI_MODE mode)
{
	static const char frac_code[8] = {0, 3, 2, 4, 1, 5, 6, 7};
	unsigned long div = (24000000 + (baudrate >> 1)) / baudrate;
	unsigned long div_code = (frac_code[div & 7] << 14) | (div >> 3);
	
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_VENDOR | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = FTDI_REQUEST_SET_BAUDRATE,
		.Value = div_code & 0xFFFF, .Index = div_code >> 16, .Length = 0 };
	int done = usb_host_ctrl_setup(func->Device, &setup, NULL, 0);
	return done;
}

static int _get_latency(FTDI_FUNCTION *func, unsigned char *plat)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_DEVICE_TO_HOST | USB_REQTYPE_VENDOR | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = FTDI_REQUEST_GET_LATENCY_TIMER,
		.Value = 0, func->Interface, .Length = 1 };
	int done = usb_host_ctrl_setup(func->Device, &setup, func->Buffer, 1);
	*plat = done ? func->Buffer[0] : 0;
	return done;
}

static int _set_latency(FTDI_FUNCTION *func, unsigned char lat)
{
	USB_REQUEST setup = (USB_REQUEST) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_VENDOR | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = FTDI_REQUEST_SET_LATENCY_TIMER,
		.Value = lat, func->Interface, .Length = 0 };
	int done = usb_host_ctrl_setup(func->Device, &setup, NULL, 0);
	return done;
}


// COM interface

static void _start(USB_HOST_FUNCTION *usb_func)
{
	FTDI_FUNCTION *func = (FTDI_FUNCTION *)usb_func;

	usb_host_start_pipe(&func->BulkOutputPipe);
	usb_host_start_pipe(&func->BulkInputPipe);

	_setup(func, FTDI_BAUDRATE, FTDI_MODE_UART);

//	unsigned char latency = 0;
//	_get_latency(func, &latency);
//	_set_latency(func, 1);
//	_get_latency(func, &latency);

	func->KernelDevice = (EXOS_TREE_DEVICE) {
		.Name = "usbftdi",	// FIXME: allow more instances
		.DeviceType = EXOS_TREE_DEVICE_COMM,
		.Device = &_comm_device,
		.Unit = 0 };	// FIXME: allow more instances
	
	exos_tree_add_device(&func->KernelDevice);
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	FTDI_FUNCTION *func = (FTDI_FUNCTION *)usb_func;

	//TODO
}

static int _open(COMM_IO_ENTRY *io)
{
	if (io->Port == 0)
	{
		// TODO: check usb function to be ready

		exos_event_set(&io->OutputEvent);
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
	// TODO
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	// TODO: implement bidirectional feature
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	FTDI_FUNCTION *func = &_func;	// FIXME: use provided usb function when opened
	if (io->Port != 0) return -1;

	int offset = 0;
	do
	{
		int part = length - offset;
		if (part > FTDI_BUFFER_SIZE) part = FTDI_BUFFER_SIZE;
		__mem_copy(func->Buffer, func->Buffer + part, buffer + offset);
		int done = usb_host_bulk_transfer(&func->BulkOutputPipe, func->Buffer, part);
		if (!done) return -1;
		offset += part;
	} while(offset < length);
	return offset;
}



