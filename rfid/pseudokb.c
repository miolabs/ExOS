#include "pseudokb.h"
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
static COMM_DEVICE _comm_device = { .Driver = &_comm_driver, .PortCount = HID_KEYBOARD_MAX_INSTANCES };

static HID_KEYBOARD_HANDLER _instances[HID_KEYBOARD_MAX_INSTANCES];
static HID_KEYBOARD_HANDLE _handles[HID_KEYBOARD_MAX_INSTANCES];
static const char *_device_names[] = { "usbkb0", "usbkb1", "usbkb2", "usbkb3" };

void keyboard_initialize()
{
	for (int i = 0; i < HID_KEYBOARD_MAX_INSTANCES; i++)
	{
		HID_KEYBOARD_HANDLE *handle = &_handles[i];
		handle->DeviceName = _device_names[i];
		handle->State = HID_KB_HANDLE_NOT_MOUNTED;
		handle->KernelDevice = (EXOS_TREE_DEVICE) {
			.Name = handle->DeviceName,
			.Device = &_comm_device,
			.Unit = i };

		_instances[i] = (HID_KEYBOARD_HANDLER) { .State = HID_KEYBOARD_NOT_PRESENT, .IOHandle = handle };
	}

	usbd_hid_add_driver(&_hidd_driver_node);
}

static HID_FUNCTION_HANDLER *_match_device(HID_FUNCTION *func)
{
	if (func->InterfaceSubClass == USB_HID_SUBCLASS_BOOT && 
		func->Protocol == USB_HID_PROTOCOL_KEYBOARD)
	{
		for (int i = 0; i < HID_KEYBOARD_MAX_INSTANCES; i++)
		{
			HID_KEYBOARD_HANDLER *kb = &_instances[i];
			if (kb->State == HID_KEYBOARD_NOT_PRESENT) 
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
	HID_KEYBOARD_HANDLER *kb = (HID_KEYBOARD_HANDLER *)handler;
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
	HID_KEYBOARD_HANDLER *kb = (HID_KEYBOARD_HANDLER *)handler;
	if (kb->Report1 != NULL)
	{
		kb->State = HID_KEYBOARD_STARTING;
		
		HID_KEYBOARD_HANDLE *handle = kb->IOHandle;
		handle->State = HID_KB_HANDLE_CLOSED;
		exos_tree_add_device(&handle->KernelDevice, "dev");	// FIXME: check re-adding the same device
	}
}

static void _stop(HID_FUNCTION_HANDLER *handler)
{
	HID_KEYBOARD_HANDLER *kb = (HID_KEYBOARD_HANDLER *)handler;
	HID_KEYBOARD_HANDLE *handle = kb->IOHandle;
	
	COMM_IO_ENTRY *io = handle->Entry;
	if (io != NULL) comm_io_close(io);
	handle->State = HID_KB_HANDLE_NOT_MOUNTED;
}

static void _notify(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data)
{
	int packet_size = ((input->Size * input->Count) >> 3);
	HID_KEYBOARD_HANDLER *kb = (HID_KEYBOARD_HANDLER *)handler;

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
				keyboard_translate(kb, key);
			else break;
		}
	}
}

void keyboard_push_text(HID_KEYBOARD_HANDLER *kb, char *text, int length)
{
	HID_KEYBOARD_HANDLE *handle = kb->IOHandle;
	if (handle != NULL && handle->State == HID_KB_HANDLE_READY)
	{
		exos_io_buffer_write(&handle->IOBuffer, text, length);
	}
}

__attribute__((__weak__))
void keyboard_translate(HID_KEYBOARD_HANDLER *kb, unsigned char key)
{
	char buffer[4];
	int length = sprintf(buffer, "%x ", key);
	keyboard_push_text(kb, buffer, length);
}


// COM interface
static int _open(COMM_IO_ENTRY *io)
{
	HID_KEYBOARD_HANDLE *handle = &_handles[io->Port];
	if (handle->State == HID_KB_HANDLE_CLOSED)
	{
		handle->State = HID_KB_HANDLE_OPENING;
		handle->Entry = io;

		// initialize input buffer for io
		exos_io_buffer_create(&handle->IOBuffer, handle->Buffer, HID_KEYBOARD_IO_BUFFER);
		handle->IOBuffer.NotEmptyEvent = &io->InputEvent;
		
        handle->State = HID_KB_HANDLE_READY;
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
	HID_KEYBOARD_HANDLE *handle = &_handles[io->Port];
	if (io == handle->Entry)
	{
		if (handle->State == HID_KB_HANDLE_READY || handle->State == HID_KB_HANDLE_ERROR)
		{
			handle->State = HID_KB_HANDLE_CLOSED;
			exos_event_reset(&io->OutputEvent);
		}
		handle->Entry = NULL;
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	HID_KEYBOARD_HANDLE *handle = &_handles[io->Port];
	if (handle->State == HID_KB_HANDLE_READY)
	{
		int done = exos_io_buffer_read(&handle->IOBuffer, buffer, length);
		return done;
	}
	return -1;
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	HID_KEYBOARD_HANDLE *handle = &_handles[io->Port];
	if (handle->State == HID_KB_HANDLE_READY)
	{
		// FAKE
		return length;
	}
	return -1;
}


