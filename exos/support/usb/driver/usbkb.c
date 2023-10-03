#include "usbkb.h"
#include <support/misc/pools.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <stdio.h>

#ifdef DEBUG
#define _verbose(level, ...) verbose(level, "usb-kb", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

#ifndef USBKB_MAX_INSTANCES
#define USBKB_MAX_INSTANCES 1
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

void usbkb_initialize()
{
	static usbkb_function_handler_t _handler_heap[USBKB_MAX_INSTANCES];

	pool_create(&_handler_pool, (node_t *)_handler_heap, sizeof(usbkb_function_handler_t), USBKB_MAX_INSTANCES);
	for (unsigned i = 0; i < USBKB_MAX_INSTANCES; i++) 
	{
		usbkb_function_handler_t *kb = (usbkb_function_handler_t *)&_handler_heap[i];
		kb->InstanceIndex = i;
		kb->UserData = NULL;
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
		usbkb_function_handler_t *kb_handler = (usbkb_function_handler_t *)pool_allocate(&_handler_pool);

		if (kb_handler != NULL)
		{
			return (hid_function_handler_t *)kb_handler;
		}
		else _verbose(VERBOSE_ERROR, "handler instance not available for new device"); 
	}
	return NULL;
}

static bool _parse_report_descriptor(usbkb_function_handler_t *handler, hid_report_parser_t *parser)
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
						//_verbose(VERBOSE_DEBUG, "input report id #%d, keyboard page, usages 0x%02x-0x%02x (offset %d)", 
						//	state->ReportId, usage_min, usage_max, state->InputOffset);
					}
				
					usage_ok = true; 
				}
				break;
			case HID_PARSE_FOUND_OUTPUT:
				//_verbose(VERBOSE_DEBUG, "output report id #%d, keyboard page, usages 0x%02x-0x%02x (offset %d)", 
				//	state->ReportId, usage_min, usage_max, state->InputOffset);
				break;
			default:
				// nothing
				break;
		}
	}

	return usage_ok;
}

static bool _start(hid_function_handler_t *handler, hid_report_parser_t *parser)
{
	usbkb_function_handler_t *kb = (usbkb_function_handler_t *)handler;

	_verbose(VERBOSE_COMMENT, "starting...");
	
#ifdef DEBUG
	bool usage_ok = _parse_report_descriptor(kb, parser);
	if(!usage_ok)
		_verbose(VERBOSE_COMMENT, "could not find keyboard usage"); 
#endif

	bool done = usb_hid_set_idle(handler->Function, 0, 0);
	if (!done) 
		_verbose(VERBOSE_ERROR, "set report idle failed!"); 

	if (done)
	{
		unsigned char leds[] = { 0x0f };
		usb_hid_set_report(handler->Function, USB_HID_REPORT_OUTPUT, 0, leds, sizeof(leds));

		usbkb_connected(kb, true);
	}
	return done; 
}

static void _stop(hid_function_handler_t *handler)
{
	usbkb_function_handler_t *kb = (usbkb_function_handler_t *)handler;
	
	usbkb_connected(kb, false);

	_verbose(VERBOSE_COMMENT, "stopped");

	pool_free(&_handler_pool, &handler->Node);
}

static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length)
{
	usbkb_function_handler_t *kb = (usbkb_function_handler_t *)handler;

#if 0
	static unsigned char buffer[32];
	sprintf(buffer, "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	_verbose(VERBOSE_DEBUG, "notify (%d) %s", length, buffer); 
#endif

	if (length >= 2)
	{
		unsigned char rp = data[0];
		usbkb_modifier_t mask = data[1];
		unsigned keys = 0;
		for (unsigned i = 2; i < length; i++)
		{
			if (data[i] != 0) keys++;
			else break;
		}

		usbkb_translate(kb, mask, &data[2], keys);
	}
}

//void usbkb_push_text(USB_KEYBOARD_HANDLER *kb, char *text, int length)
//{
//	USBKB_IO_HANDLE *handle = kb->IOHandle;
//	if (handle != NULL && handle->State == USBKB_IO_READY)
//	{
//		exos_io_buffer_write(&handle->IOBuffer, text, length);
//	}
//}

__attribute__((__weak__))
void usbkb_connected(usbkb_function_handler_t *kb, bool connected)
{
	_verbose(VERBOSE_DEBUG, "handler: %s", connected ? "connected" : "disconnected");
}

__attribute__((__weak__))
void usbkb_translate(usbkb_function_handler_t *kb, usbkb_modifier_t mask, unsigned char *keys, unsigned char length)
{
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

