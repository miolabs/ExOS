#include "usbkb.h"
#include <support/misc/pools.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <stdio.h>

#ifdef DEBUG
#define _verbose(level, ...) verbose(level, "usb-kb", __VA_ARGS__)
#else
#devine _verbose(level, ...) { /* nothing */ }
#endif

static pool_t _handler_pool;

static hid_function_handler_t *_match_device(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
		usb_hid_descriptor_t *hid_desc);
static bool _start(hid_function_handler_t *handler, hid_report_parser_t *parser);
static void _stop(hid_function_handler_t *handler);
static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length);
static const hid_driver_t _driver = {
	.MatchDevice = _match_device, 
	.Start = _start, .Stop = _stop,
	.Notify = _notify };

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
//static int _get_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value);
//static int _set_attr(io_entry_t *io, COMM_ATTR_ID attr, void *value);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
//	.GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };

void usbkb_initialize()
{
	static usb_kb_function_handler_t _handler_heap[USB_KEYBOARD_MAX_INSTANCES];

	pool_create(&_handler_pool, (node_t *)_handler_heap, sizeof(usb_kb_function_handler_t), USB_KEYBOARD_MAX_INSTANCES);
	for (unsigned i = 0; i < USB_KEYBOARD_MAX_INSTANCES; i++) 
	{
		usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)&_handler_heap[i];
		kb->InstanceIndex = i;
		kb->State = USB_KB_DETACHED;
		kb->IOEntry = NULL; 
		sprintf(kb->DeviceName, "usbkb%d", i);
		exos_io_add_device(&kb->Device, kb->DeviceName, &_io_driver, kb);
	}

	static hid_driver_node_t node = (hid_driver_node_t) { .Driver = &_driver };
	usb_hid_add_driver(&node);
}

static hid_function_handler_t *_match_device(usb_host_device_t *device, 
	usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
	usb_hid_descriptor_t *hid_desc)
{
	if (if_desc->InterfaceSubClass == USB_HID_SUBCLASS_BOOT && 
		if_desc->Protocol == USB_HID_PROTOCOL_KEYBOARD)
	{
		usb_kb_function_handler_t *kb_handler = (usb_kb_function_handler_t *)pool_allocate(&_handler_pool);

		if (kb_handler != NULL)
		{
			kb_handler->State = USB_KB_STARTING;
			return (hid_function_handler_t *)kb_handler;
		}
		else _verbose(VERBOSE_ERROR, "handler instance not available for new device"); 
	}
	return NULL;
}

static bool _parse_report_descriptor(usb_kb_function_handler_t *handler, hid_report_parser_t *parser)
{
	hid_report_parser_global_state_t *state = parser->Global;
	ASSERT(state != NULL, KERNEL_ERROR_NULL_POINTER);

	hid_report_parser_item_t item;
	hid_parse_found_t found;
	bool usage_ok = false;

	unsigned usage_offset = 0;
	usb_hid_desktop_usage_t usage = 0, usage_min = 0, usage_max = 0;

	while(usb_hid_parse_report_descriptor(parser, &item, &found))
	{
		if (parser->Global->ReportId > handler->MaxReportId)
			handler->MaxReportId = parser->Global->ReportId;

		switch(found)
		{
			case HID_PARSE_FOUND_LOCAL:
				usb_hid_parse_local_item(&item, &usage, &usage_min, &usage_max);
				break;
			case HID_PARSE_FOUND_INPUT:
				if (state->UsagePage == USB_HID_USAGE_PAGE_KEYBOARD)
				{
					if (usage_min != usage_max)
					{
						_verbose(VERBOSE_DEBUG, "report id #%d, keyboard page, usages 0x%02x-0x%02x (offset %d)", 
							state->ReportId, usage_min, usage_max, state->InputOffset);
					}
				
					usage_ok = true; 
				}
				break;
		}
	}

	return usage_ok;
}

static bool _start(hid_function_handler_t *handler, hid_report_parser_t *parser)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)handler;

	_verbose(VERBOSE_COMMENT, "starting...");
	
#ifdef DEBUG
	bool usage_ok = _parse_report_descriptor(kb, parser);
	if(!usage_ok)
		_verbose(VERBOSE_COMMENT, "could not find keyboard usage"); 
#endif

	bool done = usb_hid_set_idle(handler->Function, 0, 0);
	if (!done) 
		_verbose(VERBOSE_ERROR, "set report idle failed!"); 

	kb->State = USB_KB_READY;
}

static void _stop(hid_function_handler_t *handler)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)handler;
	
	if (kb->State == USB_KB_OPEN)
	{
		_verbose(VERBOSE_DEBUG, "closing kb io on detaching..."); 
		ASSERT(kb->IOEntry != NULL, KERNEL_ERROR_KERNEL_PANIC);
		exos_io_close(kb->IOEntry);
	}
	ASSERT(kb->IOEntry == NULL, KERNEL_ERROR_KERNEL_PANIC);
	kb->State = USB_KB_DETACHED;
	_verbose(VERBOSE_COMMENT, "stopped");

	pool_free(&_handler_pool, &handler->Node);
}

static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)handler;

//	int packet_size = ((input->Size * input->Count) >> 3);
//	USB_KEYBOARD_HANDLER *kb = (USB_KEYBOARD_HANDLER *)handler;

#if 1
	static unsigned char buffer[32];
	sprintf(buffer, "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	_verbose(VERBOSE_DEBUG, "notify (%d) %s", length, buffer); 
#endif

	//if (input == kb->Report0)
	//{
	//	kb->Modifiers = data[0];
	//}
	//else if (input == kb->Report1)
	//{
	//	for (int i = 0; i < packet_size; i++)
	//	{
	//		unsigned char key = data[i];
	//		if (key != 0)
	//			usbkb_translate(kb, key);
	//		else break;
	//	}
	//}
}

//void usbkb_push_text(USB_KEYBOARD_HANDLER *kb, char *text, int length)
//{
//	USBKB_IO_HANDLE *handle = kb->IOHandle;
//	if (handle != NULL && handle->State == USBKB_IO_READY)
//	{
//		exos_io_buffer_write(&handle->IOBuffer, text, length);
//	}
//}

//__attribute__((__weak__))
//void usbkb_translate(USB_KEYBOARD_HANDLER *kb, unsigned char key)
//{
//	char buffer[4];
//	int length = sprintf(buffer, "%x ", key);
//	usbkb_push_text(kb, buffer, length);
//}


//  IO interface

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)io->DriverContext;
	ASSERT(kb != NULL, KERNEL_ERROR_NULL_POINTER);

	switch(kb->State)
	{
		case USB_KB_READY:
			exos_io_buffer_create(&kb->IOBuffer, kb->Buffer, sizeof(kb->Buffer), &io->InputEvent, NULL);
			kb->State = USB_KB_OPEN;
			return IO_OK;
		case USB_KB_OPEN:
			return IO_ERROR_ALREADY_LOCKED;
		case USB_KB_DETACHED:
			return IO_ERROR_DEVICE_NOT_MOUNTED;
	}
	return IO_ERROR_UNKNOWN;
}

//static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
//{
//	// TODO: support baud rate, etc.
//	return -1;
//}

//static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
//{
//	// TODO: support baud rate, etc.
//	return -1;
//}

static void _close(io_entry_t *io)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)io->DriverContext;
	ASSERT(kb != NULL, KERNEL_ERROR_NULL_POINTER);

	if (kb->State == USB_KB_OPEN)
	{
		kb->State = USB_KB_READY;
		ASSERT(kb->IOEntry != NULL, KERNEL_ERROR_NULL_POINTER);
		exos_event_reset(&io->OutputEvent);
		exos_event_set(&io->InputEvent);
		kb->IOEntry = NULL;
	}
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)io->DriverContext;
	ASSERT(kb != NULL, KERNEL_ERROR_NULL_POINTER);

	if (kb->IOEntry == io && kb->State == USB_KB_OPEN)
	{
		int done = exos_io_buffer_read(&kb->IOBuffer, buffer, length);
		return done;
	}
	return -1;
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	usb_kb_function_handler_t *kb = (usb_kb_function_handler_t *)io->DriverContext;
	ASSERT(kb != NULL, KERNEL_ERROR_NULL_POINTER);

	if (kb->IOEntry == io && kb->State == USB_KB_OPEN)
	{
		// FAKE writing
		return length;
	}
	return -1;
}


