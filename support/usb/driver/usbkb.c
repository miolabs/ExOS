#include "usbkb.h"
#include <stdio.h>

static HID_FUNCTION_HANDLER *_match_device(HID_FUNCTION *func);
static int _match_handler(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input);
static void _start(HID_FUNCTION_HANDLER *handler);
static void _stop(HID_FUNCTION_HANDLER *handler);
static void _notify(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data);
static const HID_DRIVER _hidd_driver = { 
	.MatchDevice = _match_device, .MatchInputHandler = _match_handler,
	.Start = _start, .Stop = _stop,
	.Notify = _notify };
static HID_DRIVER_NODE _hidd_driver_node = { .Driver = &_hidd_driver };

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
static COMM_DEVICE _comm_device = { .Driver = &_comm_driver, .PortCount = USB_KEYBOARD_MAX_INSTANCES };

static USB_KEYBOARD_HANDLER _instances[USB_KEYBOARD_MAX_INSTANCES];
static USBKB_IO_HANDLE _handles[USB_KEYBOARD_MAX_INSTANCES];
static const char *_device_names[] = { "usbkb0", "usbkb1", "usbkb2", "usbkb3" };

void usbkb_initialize()
{
	for (int i = 0; i < USB_KEYBOARD_MAX_INSTANCES; i++)
	{
		USBKB_IO_HANDLE *handle = &_handles[i];
		handle->State = USBKB_IO_NOT_MOUNTED;
		handle->KernelDevice = (EXOS_TREE_DEVICE) {
			.Name = _device_names[i],
			.Device = &_comm_device,
			.Unit = i };

		_instances[i] = (USB_KEYBOARD_HANDLER) { .State = USB_KEYBOARD_NOT_PRESENT, .IOHandle = handle };
	}

	usbd_hid_add_driver(&_hidd_driver_node);
}

static HID_FUNCTION_HANDLER *_match_device(HID_FUNCTION *func)
{
	if (func->InterfaceSubClass == USB_HID_SUBCLASS_BOOT && 
		func->Protocol == USB_HID_PROTOCOL_KEYBOARD)
	{
		for (int i = 0; i < USB_KEYBOARD_MAX_INSTANCES; i++)
		{
			USB_KEYBOARD_HANDLER *kb = &_instances[i];
			if (kb->State == USB_KEYBOARD_NOT_PRESENT ||
				kb->State == USB_KEYBOARD_REMOVED) 
			{
				kb->Report0 = NULL;
				kb->Report1 = NULL;
				return (HID_FUNCTION_HANDLER *)kb;
			}
		}
	}
	return NULL;
}

static int _match_handler(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input)
{
	USB_KEYBOARD_HANDLER *kb = (USB_KEYBOARD_HANDLER *)handler;
	if (input->UsagePage == 7) // FIXME: use enum
	{
		if (input->Offset == 0)
		{
			kb->Report0 = input;
			return 1;
		}
		else if (kb->Report1 == 0)
		{
			kb->Report1 = input; 
			return 1;
		}
	}
	return 0;
}

static void _start(HID_FUNCTION_HANDLER *handler)
{
	USB_KEYBOARD_HANDLER *kb = (USB_KEYBOARD_HANDLER *)handler;
	if (kb->Report1 != NULL)
	{
		USBKB_IO_HANDLE *handle = kb->IOHandle;
		if (handle->State == USBKB_IO_NOT_MOUNTED)
		{
			handle->State = USBKB_IO_CLOSED;
			comm_add_device(&handle->KernelDevice, "dev");
		}
		else
		{
			handle->State = USBKB_IO_CLOSED;
		}
	}
}

static void _stop(HID_FUNCTION_HANDLER *handler)
{
	USB_KEYBOARD_HANDLER *kb = (USB_KEYBOARD_HANDLER *)handler;
	USBKB_IO_HANDLE *handle = kb->IOHandle;
	
	COMM_IO_ENTRY *io = handle->Entry;
	if (io != NULL) 
	{
		handle->State = USBKB_IO_ERROR;
		comm_io_close(io);
	}
	kb->State = USB_KEYBOARD_REMOVED;
}

static void _notify(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data)
{
	int packet_size = ((input->Size * input->Count) >> 3);
	USB_KEYBOARD_HANDLER *kb = (USB_KEYBOARD_HANDLER *)handler;

	if (input == kb->Report0)
	{
		kb->Modifiers = data[0];
	}
	else if (input == kb->Report1)
	{
		for (int i = 0; i < packet_size; i++)
		{
			unsigned char key = data[i];
			if (key != 0)
				usbkb_translate(kb, key);
			else break;
		}
	}
}

void usbkb_push_text(USB_KEYBOARD_HANDLER *kb, char *text, int length)
{
	USBKB_IO_HANDLE *handle = kb->IOHandle;
	if (handle != NULL && handle->State == USBKB_IO_READY)
	{
		exos_io_buffer_write(&handle->IOBuffer, text, length);
	}
}

__attribute__((__weak__))
void usbkb_translate(USB_KEYBOARD_HANDLER *kb, unsigned char key)
{
	char buffer[4];
	int length = sprintf(buffer, "%x ", key);
	usbkb_push_text(kb, buffer, length);
}


// COM interface
static int _open(COMM_IO_ENTRY *io)
{
	USBKB_IO_HANDLE *handle = &_handles[io->Port];
	if (handle->State == USBKB_IO_CLOSED)
	{
		handle->State = USBKB_IO_OPENING;
		handle->Entry = io;

		// initialize input buffer for io
		exos_io_buffer_create(&handle->IOBuffer, handle->Buffer, sizeof(handle->Buffer));
		handle->IOBuffer.NotEmptyEvent = &io->InputEvent;
		
        handle->State = USBKB_IO_READY;
		exos_event_set(&io->OutputEvent);
		return 0;
	}
	return -1;
}

static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	// TODO: support baud rate, etc.
	return -1;
}

static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	// TODO: support baud rate, etc.
	return -1;
}

static void _close(COMM_IO_ENTRY *io)
{
	USBKB_IO_HANDLE *handle = &_handles[io->Port];
	if (handle->Entry == io)
	{
		if (handle->State == USBKB_IO_READY)
			handle->State = USBKB_IO_CLOSED;

		exos_event_reset(&io->OutputEvent);
		exos_event_set(&io->InputEvent);
		handle->Entry = NULL;
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	USBKB_IO_HANDLE *handle = &_handles[io->Port];
	if (handle->Entry == io && handle->State == USBKB_IO_READY)
	{
		int done = exos_io_buffer_read(&handle->IOBuffer, buffer, length);
		return done;
	}
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	USBKB_IO_HANDLE *handle = &_handles[io->Port];
	if (handle->Entry == io && handle->State == USBKB_IO_READY)
	{
		// FAKE writing
		return length;
	}
	return -1;
}


