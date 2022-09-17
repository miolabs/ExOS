#include "iap.h"
#include "iap_core.h"
#include <support/usb/driver/hid.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#ifdef IAP_DEBUG
#define _verbose(level, ...) verbose(level, "iAP", __VA_ARGS__)
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

static iap_hid_handler_t _instance;	// NOTE: single instance

void iap_initialize()
{
	iap_core_initialize();

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
			iap_hid_handler_t *iap = &_instance;
			iap->InputsCount = 0;
			iap->Offset = 0;
			return (hid_function_handler_t *)iap;
		}
	}
	return NULL;
}

static bool _parse_report_descriptor(iap_hid_handler_t *iap, hid_report_parser_t *parser)
{
	hid_report_parser_global_state_t *state = parser->Global;
	ASSERT(state != NULL, KERNEL_ERROR_NULL_POINTER);

	hid_report_parser_item_t item;
	hid_parse_found_t found;
	bool usage_ok = false;

	unsigned usage_offset = 0;
//	usb_hid_desktop_usage_t usage = 0, usage_min = 0, usage_max = 0;

	while(usb_hid_parse_report_descriptor(parser, &item, &found))
	{
		//if (parser->Global->ReportId > iap->Hid.MaxReportId)
		//	iap->Hid.MaxReportId = parser->Global->ReportId;

		switch(found)
		{
			case HID_PARSE_FOUND_LOCAL:
//				usb_hid_parse_local_item(&item, &usage, &usage_min, &usage_max);
				break;
			case HID_PARSE_FOUND_OUTPUT:
				if (iap->InputsCount < IAP_MAX_REPORT_INPUTS)
				{
					struct iap_input_report *input = &iap->Inputs[iap->InputsCount ++];
					*input = (struct iap_input_report) { 
						.ReportId = state->ReportId, .Length = ((state->ReportCount * state->ReportSize) + 7) >> 3 };
					_verbose(VERBOSE_DEBUG, "report id #%d, %d bytes", input->ReportId, input->Length);
				}
				break;
		}
	}

	return usage_ok;
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

	iap_hid_handler_t *iap = (iap_hid_handler_t *)handler;
	_parse_report_descriptor(iap, parser);

	iap_core_start();
}

static void _stop(hid_function_handler_t *handler)
{
	iap_hid_handler_t *iap = (iap_hid_handler_t *)handler;
	iap_core_stop();
	iap->InputsCount = 0;
	iap->Hid.Function = NULL;
}

static void _notify(hid_function_handler_t *handler, unsigned char *data, unsigned length)
{
	iap_hid_handler_t *iap = (iap_hid_handler_t *)handler;

	unsigned offset = 0;
	unsigned char report_id = data[offset++];
//	int packet_size = ((input->Size * input->Count) >> 3);
	unsigned char lcb = data[offset++];

	if (!(lcb & IAP_LCB_CONTINUATION))
		iap->Offset = 0;
	
	int fit = length - offset;
	if ((iap->Offset + fit) > sizeof(iap->Buffer))
		fit = sizeof(iap->Buffer) - iap->Offset;
	for (int i = 0; i < fit; i++) iap->Buffer[iap->Offset++] = data[offset++];

	if (!(lcb & IAP_LCB_MORE_TO_FOLLOW))
	{
		iap_core_parse(iap->Buffer, iap->Offset);
		iap->Offset = sizeof(iap->Buffer);
	}
}

static struct iap_input_report *_get_best_smaller_report(iap_hid_handler_t *iap, unsigned size)
{
	struct iap_input_report  *best = NULL;
	unsigned best_size = 0;
	for(unsigned i = 0; i < iap->InputsCount; i++)
	{
		struct iap_input_report  *input = &iap->Inputs[i];
		unsigned report_size = input->Length;
		if (report_size <= size && 
			report_size > best_size)
		{
			best_size = report_size;
			best = input;
			if (best_size == size) break;
		}
	}
	return best;
}

static struct iap_input_report  *_get_best_bigger_report(iap_hid_handler_t *iap, unsigned size)
{
	struct iap_input_report  *best = NULL;
	int best_size = 65;
	for(int i = 0; i < iap->InputsCount; i++)
	{
		struct iap_input_report  *input = &iap->Inputs[i];
		int report_size = input->Length;
		if (report_size >= size && 
			report_size < best_size)
		{
			best_size = report_size;
			best = input;
			if (best_size == size) break;
		}
	}
	return best;
}

bool iap_send_cmd(iap_cmd_t *cmd, unsigned char *data)
{
	iap_hid_handler_t *iap = &_instance; // NOTE: single instance
	_verbose(VERBOSE_DEBUG, "<- CMD%02x(%x) tr=%x",
		cmd->CommandID, cmd->LingoID, cmd->Transaction);
					
	struct iap_input_report  *input;
	unsigned char buffer[64];
	unsigned char sum = 0;
	
	buffer[1] = IAP_LCB_START;
	unsigned offset = 2;
	unsigned length = 1 + (cmd->LingoID == 4 ? 2 : 1) + 2 + cmd->Length;
	buffer[offset++] = 0x55;
	if (length > 255)
	{
		buffer[offset++] = 0;
		sum += buffer[offset++] = length >> 8;
	}
	sum += buffer[offset++] = length;

	sum += buffer[offset++] = cmd->LingoID;
	if (cmd->LingoID == 4)
		sum += buffer[offset++] = cmd->CommandID >> 8;
	sum += buffer[offset++] = cmd->CommandID;

	sum += buffer[offset++] = cmd->Transaction >> 8;
	sum += buffer[offset++] = cmd->Transaction;

	unsigned payload_offset = 0;
	unsigned payload = data != NULL ? cmd->Length : 0;
	while(payload > 0)
	{
		unsigned fit = sizeof(buffer) - offset;
		if (fit > payload) fit = payload;
		input = _get_best_smaller_report(iap, offset + fit);
		if (input == NULL) break;

		unsigned report_size = input->Length + 1;
		if ((offset + fit) > report_size) fit = report_size - offset;

		for (unsigned i = 0; i < fit; i++) 
			sum += buffer[offset++] = data[payload_offset++];

		if (offset == report_size)
		{
			buffer[0] = input->ReportId;
			buffer[1] |= IAP_LCB_MORE_TO_FOLLOW;
			if (!usb_hid_set_report(iap->Hid.Function, USB_HID_REPORT_OUTPUT, input->ReportId, buffer, offset))
			{
				_verbose(VERBOSE_ERROR, "set report failed (1)");
				return false;
			}

			buffer[1] = IAP_LCB_CONTINUATION;
			offset = 2;
		}

		payload -= fit;
	}

	for (unsigned i = 0; i < payload; i++) 
		sum += buffer[offset++] = data[payload_offset++];
	buffer[offset++] = -sum;

	input = _get_best_bigger_report(iap, offset);
	if (input != NULL)
	{
		unsigned report_size = input->Length + 1;
		buffer[0] = input->ReportId;
		for (unsigned i = offset; i < report_size; i++) 
			buffer[i] = 0; 
		
		if (usb_hid_set_report(iap->Hid.Function, USB_HID_REPORT_OUTPUT, input->ReportId, buffer, report_size))
		{
			return true;
		}
		else _verbose(VERBOSE_DEBUG, "set report failed (2)");
	}
	else _verbose(VERBOSE_ERROR, "no bigger input report is ready to send command");
	return false;
}


