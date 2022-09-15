#include "iap2_hidd.h"

#include <support/usb/driver/hid.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#ifdef IAP_DEBUG
#define _verbose(level, ...) verbose(level, "iAP2-hidd", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif

static hid_function_handler_t *_match_device(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
		usb_hid_descriptor_t *hid_desc);
static bool _start(hid_function_handler_t *handler, hid_report_parser_t *parser);
static void _stop(hid_function_handler_t *handler);
static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length);
static const hid_driver_t _hid_driver = { 
	.MatchDevice = _match_device, 
	.Start = _start, .Stop = _stop,
	.Notify = _notify };
static hid_driver_node_t _hid_driver_node = { .Driver = &_hid_driver };

static bool _iap2_send(iap2_transport_t *t, const unsigned char *data, unsigned length);
static const iap2_transport_driver_t _iap2_driver = {
	.Send = _iap2_send,
	}; 

static iap2_hid_handler_t _instance;	// NOTE: single instance

void iap2_hidd_initialize()
{
	iap2_initialize();

	usb_hid_add_driver(&_hid_driver_node);
}

static hid_function_handler_t *_match_device(usb_host_device_t *device, usb_configuration_descriptor_t *conf_desc, usb_interface_descriptor_t *if_desc, 
		usb_hid_descriptor_t *hid_desc)
{
	if (device->Vendor == 0x05ac // Apple
		&& (device->Product & 0xff00) == 0x1200) // iAP2 apple device
	{
		if (_instance.Hid.Function == NULL)		// NOTE: check if our instance is not running
		{
			iap2_hid_handler_t *iap2 = &_instance;
			iap2->ReportsCount = 0;
			return &iap2->Hid;
		}
	}
	return NULL;
}

static bool _parse_report_descriptor(iap2_hid_handler_t *iap2, hid_report_parser_t *parser)
{
	hid_report_parser_global_state_t *state = parser->Global;
	ASSERT(state != NULL, KERNEL_ERROR_NULL_POINTER);

	hid_report_parser_item_t item;
	hid_parse_found_t found;

	while(usb_hid_parse_report_descriptor(parser, &item, &found))
	{
		switch(found)
		{
			case HID_PARSE_FOUND_OUTPUT:	// NOTE: output, ok? :)
				if (iap2->ReportsCount < IAP2_MAX_REPORTS)
				{
					struct iap2_hid_report *r = &iap2->Reports[iap2->ReportsCount ++];
					*r = (struct iap2_hid_report) { 
						.ReportId = state->ReportId, .Length = ((state->ReportCount * state->ReportSize) + 7) >> 3 };
					_verbose(VERBOSE_DEBUG, "report id #%d, %d bytes", r->ReportId, r->Length);
				}
				break;
		}
	}

	return iap2->ReportsCount != 0;
}

static bool _start(hid_function_handler_t *handler, hid_report_parser_t *parser)
{
#ifdef IAP_ENABLE_IPAD_CHARGING
	usb_request_t setup = (usb_request_t) {
		.RequestType = USB_REQTYPE_HOST_TO_DEVICE | USB_REQTYPE_VENDOR | USB_REQTYPE_RECIPIENT_DEVICE,
		.RequestCode = USB_IAP_REQ_DEVICE_POWER_REQUEST,
		.Value = 0x6400, .Index = 0x6400, .Length = 0 };
	bool done = usb_host_ctrl_setup(handler->Function->Device, &setup, NULL, 0);
	if (!done) _verbose(VERBOSE_ERROR, "DevicePowerRequest failed!");
#endif

	iap2_hid_handler_t *iap2 = (iap2_hid_handler_t *)handler;
	_parse_report_descriptor(iap2, parser);

	iap2_transport_create(&iap2->Transport, "Hid - Device Mode", &_iap2_driver);
	iap2_start(&iap2->Transport);
}

static void _stop(hid_function_handler_t *handler)
{
	iap2_hid_handler_t *iap2 = (iap2_hid_handler_t *)handler;
	iap2_stop();
	iap2->ReportsCount = 0;
	iap2->Hid.Function = NULL;
}


static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length)
{
	iap2_hid_handler_t *iap2 = (iap2_hid_handler_t *)handler;

	if (length > 2)
	{
		unsigned offset = 0;
		unsigned char report_id = data[offset++];
		unsigned char lcb = data[offset++];
		_verbose(VERBOSE_DEBUG, "got input report #%d, %d bytes", report_id, length); 

		unsigned part = length - offset;

		if (!(lcb & IAP2_LCB_CONTINUATION))
			iap2->InputOffset = 0;

		unsigned fit = length - offset;
		if ((iap2->InputOffset + fit) <= sizeof(iap2->InputBuffer))
		{
			unsigned input_length = iap2->InputOffset;
			for (unsigned i = 0; i < fit; i++) iap2->InputBuffer[input_length++] = data[offset++];

			if (!(lcb & IAP2_LCB_MORE_TO_FOLLOW))
			{
				iap2_parse(&iap2->Transport, iap2->InputBuffer, input_length);
				iap2->InputOffset = sizeof(iap2->InputBuffer);
			}
			else 
			{
				iap2->InputOffset = input_length;
			}
		}
		else 
		{
			_verbose(VERBOSE_ERROR, "packet doesn't fit in buffer, lost!");
			iap2->InputOffset = sizeof(iap2->InputBuffer);
		}
	}
	else _verbose(VERBOSE_ERROR, "got short report!");
}

static struct iap2_hid_report *_get_best_smaller_report(iap2_hid_handler_t *iap2, unsigned size)
{
	struct iap2_hid_report *best = NULL;
	unsigned best_size = 0;
	for(unsigned i = 0; i < iap2->ReportsCount; i++)
	{
		struct iap2_hid_report  *r = &iap2->Reports[i];
		unsigned report_size = r->Length;
		if (report_size <= size && 
			report_size > best_size)
		{
			best_size = report_size;
			best = r;
			if (best_size == size) break;
		}
	}
	return best;
}

static struct iap2_hid_report  *_get_best_bigger_report(iap2_hid_handler_t *iap2, unsigned size)
{
	struct iap2_hid_report *best = NULL;
	unsigned best_size = 65;
	for(unsigned i = 0; i < iap2->ReportsCount; i++)
	{
		struct iap2_hid_report  *r = &iap2->Reports[i];
		unsigned report_size = r->Length;
		if (report_size >= size && 
			report_size < best_size)
		{
			best_size = report_size;
			best = r;
			if (best_size == size) break;
		}
	}
	return best;
}

static bool _iap2_send(iap2_transport_t *t, const unsigned char *data, unsigned length)
{
	iap2_hid_handler_t *iap2 = &_instance; // NOTE: single instance
	ASSERT(t == &iap2->Transport, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(data != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(length != 0, KERNEL_ERROR_NOT_SUPPORTED);

	struct iap2_hid_report *r;
	unsigned char buffer[64];
	
	buffer[1] = IAP2_LCB_START;
	unsigned offset = 2;

	unsigned payload_offset = 0;
	unsigned payload = data != NULL ? length : 0;
	while(payload > 0)
	{
		unsigned fit = sizeof(buffer) - offset;
		if (fit > payload) fit = payload;
		r = _get_best_smaller_report(iap2, offset + fit);
		if (r == NULL) break;

		unsigned report_size = r->Length + 1; // NOTE: byte for report_id
		if ((offset + fit) > report_size) fit = report_size - offset;

		for (unsigned i = 0; i < fit; i++) 
			buffer[offset++] = data[payload_offset++];

		if (offset == report_size)
		{
			buffer[0] = r->ReportId;
			buffer[1] |= IAP2_LCB_MORE_TO_FOLLOW;
			if (!usb_hid_set_report(iap2->Hid.Function, USB_HID_REPORT_OUTPUT, r->ReportId, buffer, offset))
			{
				_verbose(VERBOSE_ERROR, "set report failed (1)");
				return false;
			}

			buffer[1] = IAP2_LCB_CONTINUATION;
			offset = 2;
		}

		payload -= fit;
	}

	r = _get_best_bigger_report(iap2, offset);
	if (r != NULL)
	{
		unsigned report_size = r->Length + 1;	// NOTE: byte for report_id
		buffer[0] = r->ReportId;

		while (payload_offset < length)
			buffer[offset++] = data[payload_offset++];
		ASSERT(offset <= sizeof(buffer), KERNEL_ERROR_UNKNOWN);
		for (unsigned i = offset; i < report_size; i++) 
			buffer[i] = 0; 
		
		if (usb_hid_set_report(iap2->Hid.Function, USB_HID_REPORT_OUTPUT, r->ReportId, buffer, report_size))
		{
			return true;
		}
		else _verbose(VERBOSE_DEBUG, "set report failed (2)");
	}
	else _verbose(VERBOSE_ERROR, "no bigger input report is ready to send command");
	return false;
}


