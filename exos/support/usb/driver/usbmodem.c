#include "usbmodem.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>
#include <kernel/dispatch.h>
#include <usb/classes/msc.h>

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static EXOS_DISPATCHER_CONTEXT _dispatcher_context;
static void _dispatch_open(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static void _dispatch_close(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static void _dispatch_io(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);

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
static unsigned char _io_buffer[USBMODEM_IO_BUFFER] __attribute__((aligned(16)));

static USBMODEM_FUNCTION _func __usb; // NOTE: single instance support
static char _str_desc[64] __usb;

void usbmodem_initialize()
{
	_func = (USBMODEM_FUNCTION) { .State = USBMODEM_NOT_ATTACHED };

	EXOS_EVENT event;
	exos_event_create(&event);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, NULL, _service, &event);
	exos_event_wait(&event, EXOS_TIMEOUT_NEVER);

	_driver_node = (USB_HOST_FUNCTION_DRIVER_NODE) { .Driver = &_driver };
	usb_host_driver_register(&_driver_node);
}

static int _device_if_matches(USB_HOST_DEVICE *device, USB_INTERFACE_DESCRIPTOR *if_desc)
{
	if (device->Vendor == 0x12d1 && device->Product == 0x1446)	// mass storage controller
		return 1;

	if (device->Vendor == 0x05c6 && device->Product == 0x6000)	// modem device
	{
		if (if_desc->InterfaceNumber == 0)
			return 2;

//		if (if_desc->InterfaceNameIndex != 0 && 
//			usb_host_read_string_descriptor(device, 0x409, if_desc->InterfaceNameIndex, _str_desc, sizeof(_str_desc)))
//		{
//			usb_desc2str((USB_DESCRIPTOR_HEADER *)_str_desc, _str_desc, sizeof(_str_desc));
//			return 2;
//		}
	}
	if (device->Vendor == 0x12d1 && device->Product == 0x14ac) // modem device Huawei
	{
		if (if_desc->InterfaceNumber == 0)
			return 2;
	}
        if (device->Vendor == 0x0519 && device->Product == 0x1001) // CDC Star: barcode scanner
        {
                if (if_desc->InterfaceNumber == 0)
                    return 2;
        }

	return 0;
}

static USB_HOST_FUNCTION *_check_interface(USB_HOST_DEVICE *device, USB_CONFIGURATION_DESCRIPTOR *conf_desc, USB_DESCRIPTOR_HEADER *fn_desc)
{
	USB_ENDPOINT_DESCRIPTOR *in_desc;
	USB_ENDPOINT_DESCRIPTOR *out_desc;
	USB_ENDPOINT_DESCRIPTOR *int_desc;

	if (fn_desc->DescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE)
	{
		USB_INTERFACE_DESCRIPTOR *if_desc = (USB_INTERFACE_DESCRIPTOR *)fn_desc;

		USBMODEM_FUNCTION *func = &_func;
		if (func->State == USBMODEM_NOT_ATTACHED)
		{
			switch(_device_if_matches(device, if_desc))
			{
				case 1: // mass storage controller
					usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);

					out_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
					in_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
					if (!in_desc || !out_desc) return NULL;

					usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, out_desc);
					usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, in_desc);

					func->Interface = if_desc->InterfaceNumber;
//					func->Protocol = proto;
					func->State = USBMODEM_ATTACHING_PRE;
					return (USB_HOST_FUNCTION *)func;
				case 2:	// CDC modem
					usb_host_create_function((USB_HOST_FUNCTION *)func, device, &_driver);

					out_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_HOST_TO_DEVICE, 0);
					in_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_BULK, USB_DEVICE_TO_HOST, 0);
					int_desc = usb_enumerate_find_endpoint_descriptor(conf_desc, if_desc, USB_TT_INTERRUPT, USB_DEVICE_TO_HOST, 0);
					if (!in_desc || !out_desc) return NULL;

					usb_host_init_pipe_from_descriptor(device, &func->BulkOutputPipe, out_desc);
					usb_host_init_pipe_from_descriptor(device, &func->BulkInputPipe, in_desc);
					if (int_desc != NULL) usb_host_init_pipe_from_descriptor(device, &func->InterruptPipe, int_desc);

					func->Interface = if_desc->InterfaceNumber;
					func->State = USBMODEM_ATTACHING;
					return (USB_HOST_FUNCTION *)func;
					break;
			}
		}
	}
	return NULL;
}

static void _switch(USBMODEM_FUNCTION *func)
{
	USB_MSC_CBW *cbw = (USB_MSC_CBW *)func->OutputBuffer;
//	*cbw = (USB_MSC_CBW) { .Signature = USB_MSC_CBW_SIGNATURE,
//		.Tag = 0xF019BD30,
//		.DataTransferLength = 0xC0,
//		.Flags = 0x80, .LUN = 0, .CDBLength = 6, 
//		.CDB = { 0x71, 0x03 } };

	*cbw = (USB_MSC_CBW) { .Signature = USB_MSC_CBW_SIGNATURE,
		.CDB = { 0x11, 0x06 } };

	int done = usb_host_bulk_transfer(&func->BulkOutputPipe, cbw, sizeof(USB_MSC_CBW), EXOS_TIMEOUT_NEVER);
}


static void _start(USB_HOST_FUNCTION *usb_func)
{
	USBMODEM_FUNCTION *func = (USBMODEM_FUNCTION *)usb_func;

	switch(func->State)
	{
		case USBMODEM_ATTACHING_PRE:
			usb_host_start_pipe(&func->BulkOutputPipe);
			usb_host_start_pipe(&func->BulkInputPipe);

			func->Entry = NULL;
			func->State = USBMODEM_CLOSED;
			_switch(func);
			break;
		case USBMODEM_ATTACHING:
			usb_host_start_pipe(&func->BulkOutputPipe);
			usb_host_start_pipe(&func->BulkInputPipe);
			// NOTE: we should start the interrupt ep now

			if (exos_tree_find_path(NULL, "dev/usbmodem") == NULL)
			{
				func->KernelDevice = (EXOS_TREE_DEVICE) {
					.Name = "usbmodem",	
					.Device = &_comm_device,
					.Unit = 0 };
		
				comm_add_device(&func->KernelDevice, "dev");
			}
			func->Entry = NULL;
			func->State = USBMODEM_CLOSED;
			break;
		default:
			kernel_panic(KERNEL_ERROR_UNKNOWN);
			break;
	}
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	USBMODEM_FUNCTION *func = (USBMODEM_FUNCTION *)usb_func;

	usb_host_stop_pipe(&func->BulkOutputPipe);
	usb_host_stop_pipe(&func->BulkInputPipe);
	// NOTE: we should stop the interrupt ep now

	COMM_IO_ENTRY *io = func->Entry;
	if (io != NULL) comm_io_close(io);

	func->State = USBMODEM_NOT_ATTACHED;
}

static int _open(COMM_IO_ENTRY *io)
{
	USBMODEM_FUNCTION *func = &_func;
	if (func->State == USBMODEM_CLOSED && func->Entry == NULL)
	{
		func->State = USBMODEM_OPENING;
		func->Entry = io;

		// initialize input buffer for io
		exos_io_buffer_create(&func->IOBuffer, _io_buffer, USBMODEM_IO_BUFFER);
		func->IOBuffer.NotEmptyEvent = &io->InputEvent;

		EXOS_DISPATCHER dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch_open, .Event = NULL, .CallbackState = func };
		exos_dispatcher_add(&_dispatcher_context, &dispatcher, 0);
		
		exos_event_wait(&io->OutputEvent, EXOS_TIMEOUT_NEVER);
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
	USBMODEM_FUNCTION *func = &_func;
	if (func->Entry == io &&
		(func->State == USBMODEM_READY || func->State == USBMODEM_ERROR))
	{
		USBMODEM_REQUEST req = { .Function = func };
		exos_event_create(&req.DoneEvent);
		EXOS_DISPATCHER dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch_close, .CallbackState = &req };
		exos_dispatcher_add(&_dispatcher_context, &dispatcher, 0);

		exos_event_wait(&req.DoneEvent, EXOS_TIMEOUT_NEVER);
		exos_event_set(&io->InputEvent);
	}
	func->Entry = NULL;
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	USBMODEM_FUNCTION *func = &_func;
	if (func->Entry == io &&
		func->State == USBMODEM_READY)
	{
		int done = exos_io_buffer_read(&func->IOBuffer, buffer, length);
		return done;
	}
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	if (io->Port == 0)
	{
		USBMODEM_FUNCTION *func = &_func;
		if (io == func->Entry)
		{
			int offset = 0;
			do
			{
				int part = length - offset;
				if (part > USBMODEM_USB_BUFFER) part = USBMODEM_USB_BUFFER;
				__mem_copy(func->OutputBuffer, func->OutputBuffer + part, buffer + offset);
				int done = usb_host_bulk_transfer(&func->BulkOutputPipe, func->OutputBuffer, part, EXOS_TIMEOUT_NEVER);
				if (!done) return -1;
				offset += part;
			} while(offset < length);
			return offset;
		}
	}
	return -1;
}

static void *_service(void *arg)
{
	exos_dispatcher_context_create(&_dispatcher_context);
	exos_event_set((EXOS_EVENT *)arg);	// notify of service ready

	while(1)
	{
		exos_dispatch(&_dispatcher_context, EXOS_TIMEOUT_NEVER);
	}
}

static void _dispatch_open(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	USBMODEM_FUNCTION *func = (USBMODEM_FUNCTION *)dispatcher->CallbackState;
    USB_REQUEST_BUFFER *urb = &func->Request;
#ifdef DEBUG
	if (urb->Status == URB_STATUS_ISSUED) kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
	usb_host_urb_create(urb, &func->BulkInputPipe);
	usb_host_begin_bulk_transfer(urb, func->InputBuffer, USBMODEM_USB_BUFFER);
	
	func->State = USBMODEM_READY;

    COMM_IO_ENTRY *io = func->Entry;
	exos_event_set(&io->OutputEvent);

   	func->Dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch_io, .Event = &urb->Event, .CallbackState = func };
	exos_dispatcher_add(&_dispatcher_context, &func->Dispatcher, EXOS_TIMEOUT_NEVER);
}

static void _dispatch_io(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	USBMODEM_FUNCTION *func = (USBMODEM_FUNCTION *)dispatcher->CallbackState;
	USB_REQUEST_BUFFER *urb = &func->Request;
	if (func->State == USBMODEM_READY)
	{
		if (urb->Status == URB_STATUS_DONE)
		{
			int done = usb_host_end_bulk_transfer(urb, EXOS_TIMEOUT_NEVER);
			if (done > 0)
			{
				exos_io_buffer_write(&func->IOBuffer, func->InputBuffer, done);
			}

			usb_host_begin_bulk_transfer(urb, func->InputBuffer, USBMODEM_USB_BUFFER);
			
			func->Dispatcher = (EXOS_DISPATCHER) { .Callback = _dispatch_io, .Event = &urb->Event, .CallbackState = func };
			exos_dispatcher_add(&_dispatcher_context, &func->Dispatcher, EXOS_TIMEOUT_NEVER);
		}
		else if (urb->Status == URB_STATUS_FAILED)
		{
			func->State = USBMODEM_ERROR;
		}
	}
}

static void _dispatch_close(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	USBMODEM_REQUEST *req = (USBMODEM_REQUEST *)dispatcher->CallbackState;
	USBMODEM_FUNCTION *func = (USBMODEM_FUNCTION *)req->Function;
	USB_REQUEST_BUFFER *urb = &func->Request;
	if (func->State == USBMODEM_READY)
	{
		usb_host_end_bulk_transfer(urb, 1000);
	}
	func->State = USBMODEM_CLOSED;
	exos_event_set(&req->DoneEvent);
}




