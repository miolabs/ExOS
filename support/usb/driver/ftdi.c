#include "ftdi.h"
#include <usb/enumerate.h>
#include <kernel/machine/hal.h>
#include <kernel/panic.h>

#ifndef FTDI_MAX_INSTANCES 
#define FTDI_MAX_INSTANCES 2
#endif

#ifndef FTDI_BAUDRATE
#define FTDI_BAUDRATE 9600
#endif

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static EXOS_EVENT _handle_event;
static EXOS_MUTEX _handle_list_lock;
static EXOS_LIST _handle_list;

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
static COMM_DEVICE _comm_device = { .Driver = &_comm_driver, .PortCount = FTDI_MAX_INSTANCES };

FTDI_FUNCTION _functions[FTDI_MAX_INSTANCES] __usb;
static volatile unsigned char _func_usage[FTDI_MAX_INSTANCES];
static FTDI_FUNCTION *_get_func(int port);
static const char *_device_names[] = { "usbftdi0", "usbftdi1", "usbftdi2", "usbftdi3" }; 

void ftdi_initialize()
{
	exos_event_create(&_handle_event);
	exos_mutex_create(&_handle_list_lock);
	list_initialize(&_handle_list);
	for(int port = 0; port < FTDI_MAX_INSTANCES; port++)
	{
		_func_usage[port] = 0;
		FTDI_FUNCTION *func = &_functions[port];
		*func = (FTDI_FUNCTION) { .DeviceUnit = port, .DeviceName = _device_names[port] }; 
	}

	EXOS_EVENT event;
	exos_event_create(&event);
	exos_thread_create(&_thread, 5, _stack, THREAD_STACK, NULL, _service, &event);
	exos_event_wait(&event, EXOS_TIMEOUT_NEVER);

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
			FTDI_FUNCTION *func = NULL;
			for(int port = 0; port < FTDI_MAX_INSTANCES; port++) if (_func_usage[port] == 0)
			{
				_func_usage[port] = 1;
				func = &_functions[port];
				break;
			}
			if (func != NULL)
			{
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
	int done = usb_host_ctrl_setup(func->Device, &setup, func->OutputBuffer, 1);
	*plat = done ? func->OutputBuffer[0] : 0;
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
		.Name = func->DeviceName,
		.DeviceType = EXOS_TREE_DEVICE_COMM,
		.Device = &_comm_device,
		.Unit = func->DeviceUnit };

	func->AsyncHandle = (FTDI_HANDLE) { .State = FTDI_HANDLE_CLOSED };

	exos_tree_add_device(&func->KernelDevice);
}

static void _stop(USB_HOST_FUNCTION *usb_func)
{
	FTDI_FUNCTION *func = (FTDI_FUNCTION *)usb_func;

	//TODO
}


// COM interface

static int _open(COMM_IO_ENTRY *io)
{
	FTDI_FUNCTION *func = _get_func(io->Port);
	if (func != NULL)
	{
		FTDI_HANDLE *handle = &func->AsyncHandle;
		if (handle->State == FTDI_HANDLE_CLOSED)
		{
			*handle = (FTDI_HANDLE) { .Entry = io };
			exos_io_buffer_create(&handle->IOBuffer, handle->Buffer, FTDI_IO_BUFFER);
			handle->IOBuffer.NotEmptyEvent = &io->InputEvent;
			exos_mutex_create(&handle->Lock);
			handle->State = FTDI_HANDLE_OPENING;
	
			exos_mutex_lock(&_handle_list_lock);
			list_add_tail(&_handle_list, (EXOS_NODE *)handle);
			exos_mutex_unlock(&_handle_list_lock);
			exos_event_set(&_handle_event);
	
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
	EXOS_EVENT event;
	exos_event_create(&event);

	FTDI_FUNCTION *func = _get_func(io->Port);
	if (func != NULL)
	{
		FTDI_HANDLE *handle = &func->AsyncHandle;
		if (handle->State == FTDI_HANDLE_READY)
		{
			exos_mutex_lock(&handle->Lock);
            handle->IOBuffer.NotEmptyEvent = NULL;
			handle->StateEvent = &event;
			handle->State = FTDI_HANDLE_CLOSING;
			exos_mutex_unlock(&handle->Lock);
			exos_event_set(&_handle_event);

			if (-1 == exos_event_wait(&event, 500))
				kernel_panic(KERNEL_ERROR_UNKNOWN);
			if (handle->State != FTDI_HANDLE_CLOSED)
				kernel_panic(KERNEL_ERROR_UNKNOWN);
		}
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	FTDI_FUNCTION *func = _get_func(io->Port);
	if (func == NULL) return -1;

	FTDI_HANDLE *handle = &func->AsyncHandle;
	//if (handle->State != FTDI_HANDLE_READY) return -1;

	int done = exos_io_buffer_read(&handle->IOBuffer, buffer, length);
	return done;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	FTDI_FUNCTION *func = _get_func(io->Port);
	if (func == NULL) return -1;

	int offset = 0;
	do
	{
		int part = length - offset;
		if (part > FTDI_USB_BUFFER) part = FTDI_USB_BUFFER;
		__mem_copy(func->OutputBuffer, func->OutputBuffer + part, buffer + offset);
		int done = usb_host_bulk_transfer(&func->BulkOutputPipe, func->OutputBuffer, part);
		if (!done) return -1;
		offset += part;
	} while(offset < length);
	return offset;
}

static FTDI_FUNCTION *_get_func(int port)
{
	if (port < FTDI_MAX_INSTANCES && _func_usage[port] != 0)
		return &_functions[port];
	return NULL;
}

static void *_service(void *arg)
{
	exos_event_set((EXOS_EVENT *)arg);	// notify of service ready

	EXOS_EVENT *events[FTDI_MAX_INSTANCES + 1];
	events[0] = &_handle_event;
	int count = 1;
	while(1)
	{
		exos_event_wait_multiple(events, count, EXOS_TIMEOUT_NEVER);
		exos_event_reset(&_handle_event);

		exos_mutex_lock(&_handle_list_lock);
		count = 1;
		EXOS_NODE *next;
		for(EXOS_NODE *node = LIST_HEAD(&_handle_list)->Succ; node != LIST_TAIL(&_handle_list); node = next)
		{
			next = node->Succ;	// take it now to allow node removal
			FTDI_HANDLE *handle = (FTDI_HANDLE *)node;
			exos_mutex_lock(&handle->Lock);
			
			COMM_IO_ENTRY *io = handle->Entry;
			FTDI_FUNCTION *func = _get_func(io->Port);
			USB_REQUEST_BUFFER *urb = &handle->Request;
			EXOS_EVENT *event = NULL;
            int done;
			switch(handle->State)
			{
				case FTDI_HANDLE_OPENING:
#ifdef DEBUG
					if (urb->Status != URB_STATUS_EMPTY) kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
					usb_host_urb_create(urb, &func->BulkInputPipe);
					usb_host_begin_bulk_transfer(urb, func->InputBuffer, FTDI_USB_BUFFER);
					event = &urb->Event;
					handle->State = FTDI_HANDLE_READY;
					break;
				case FTDI_HANDLE_READY:
					if (urb->Status == URB_STATUS_DONE)
					{
						done = usb_host_end_bulk_transfer(urb);
						if (done > 2)
						{
							exos_io_buffer_write(&handle->IOBuffer, func->InputBuffer + 2, done - 2);
						}
						usb_host_begin_bulk_transfer(urb, func->InputBuffer, FTDI_USB_BUFFER);
						event = &urb->Event;
					}
					else if (urb->Status == URB_STATUS_FAILED)
					{
						handle->State = FTDI_HANDLE_ERROR;
						list_remove(node);
					}
					break;
				case FTDI_HANDLE_CLOSING:
					done = usb_host_end_bulk_transfer(urb);
					handle->State = FTDI_HANDLE_CLOSED;
					list_remove(node);
					if (handle->StateEvent != NULL) exos_event_set(handle->StateEvent);
					break;
			}
			if (event != NULL)
				events[count++] = event;
			exos_mutex_unlock(&handle->Lock);
		}
		exos_mutex_unlock(&_handle_list_lock);

		exos_thread_sleep(10);
	}
}

