#include "iap.h"
#include "iap_core.h"
#include <support/usb/driver/hid.h>

static HID_FUNCTION_HANDLER *_match_device(HID_FUNCTION *func);
static int _match_handler(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input);
static void _start(HID_FUNCTION_HANDLER *handler);
static void _stop(HID_FUNCTION_HANDLER *handler);
static void _notify(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data);
static const HID_DRIVER _hid_driver = { 
	.MatchDevice = _match_device, .MatchInputHandler = _match_handler,
	.Start = _start, .Stop = _stop,
	.Notify = _notify };
static HID_DRIVER_NODE _hid_driver_node = { .Driver = &_hid_driver };

static IAP_HID_HANDLER _instance;	// NOTE: single instance

void iap_initialize()
{
	iap_core_initialize();

	usbd_hid_add_driver(&_hid_driver_node);
}

static HID_FUNCTION_HANDLER *_match_device(HID_FUNCTION *func)
{
	USB_HOST_DEVICE *device = func->Device;
	if (device->Vendor == 0x05ac // Apple
		&& (device->Product & 0xff00) == 0x1200) // iAP2 apple device
	{
		if (_instance.Function == NULL) 
		{
			IAP_HID_HANDLER *iap = &_instance;
			iap->InputsCount = 0;
			iap->Offset = 0;
			return (HID_FUNCTION_HANDLER *)iap;
		}
	}
	return NULL;
}

static int _match_handler(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input)
{
	IAP_HID_HANDLER *iap = (IAP_HID_HANDLER *)handler;
	if (iap->InputsCount < IAP_MAX_REPORT_INPUTS)
	{
		iap->Inputs[iap->InputsCount++] = input;
		return 1;
	}
	return 0;
}

static void _start(HID_FUNCTION_HANDLER *handler)
{
	// FIXME: check running state
	iap_core_start();
}

static void _stop(HID_FUNCTION_HANDLER *handler)
{
	IAP_HID_HANDLER *iap = (IAP_HID_HANDLER *)handler;
	iap_core_stop();
	iap->InputsCount = 0;
}

static void _notify(HID_FUNCTION_HANDLER *handler, HID_REPORT_INPUT *input, unsigned char *data)
{
	IAP_HID_HANDLER *iap = (IAP_HID_HANDLER *)handler;
	int packet_size = ((input->Size * input->Count) >> 3);
	int offset = 0;
	unsigned char lcb = data[offset++];

	if (!(lcb & IAP_LCB_CONTINUATION))
		iap->Offset = 0;
	
	int fit = packet_size - 1;
	if ((iap->Offset + fit) > sizeof(iap->Buffer))
		fit = sizeof(iap->Buffer) - iap->Offset;
	for (int i = 0; i < fit; i++) iap->Buffer[iap->Offset++] = data[offset++];

	if (!(lcb & IAP_LCB_MORE_TO_FOLLOW))
	{
		iap_core_parse(iap->Buffer, iap->Offset);
		iap->Offset = sizeof(iap->Buffer);
	}
}


static HID_REPORT_INPUT *_get_best_smaller_report(IAP_HID_HANDLER *iap, int size)
{
	HID_REPORT_INPUT *best = NULL;
	int best_size = 0;
	for(int i = 0; i < iap->InputsCount; i++)
	{
		HID_REPORT_INPUT *input = iap->Inputs[i];
		int report_size = (input->Count * input->Size) >> 3;
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

static HID_REPORT_INPUT *_get_best_bigger_report(IAP_HID_HANDLER *iap, int size)
{
	HID_REPORT_INPUT *best = NULL;
	int best_size = 65;
	for(int i = 0; i < iap->InputsCount; i++)
	{
		HID_REPORT_INPUT *input = iap->Inputs[i];
		int report_size = (input->Count * input->Size) >> 3;
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

int iap_send_cmd(IAP_CMD *cmd, unsigned char *data)
{
	IAP_HID_HANDLER *iap = &_instance; // NOTE: single instance
	HID_REPORT_INPUT *input;
	unsigned char buffer[64];
	unsigned char sum = 0;
	
	buffer[1] = IAP_LCB_START;
	int offset = 2;
	int length = 1 + (cmd->LingoID == 4 ? 2 : 1) + 2 + cmd->Length;
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

	int payload_offset = 0;
	int payload = data != NULL ? cmd->Length : 0;
	while(payload > 0)
	{
		int fit = sizeof(buffer) - offset;
		if (fit > payload) fit = payload;
		input = _get_best_smaller_report(iap, offset + fit);
		if (input == NULL) break;

		int report_size = ((input->Size * input->Count) >> 3) + 1;
		if ((offset + fit) > report_size) fit = report_size - offset;

		for (int i = 0; i < fit; i++) 
			sum += buffer[offset++] = data[payload_offset++];

		if (offset == report_size)
		{
			buffer[0] = input->ReportId;
			buffer[1] |= IAP_LCB_MORE_TO_FOLLOW;
			if (!usbd_hid_set_report(iap->Function, USB_HID_REPORT_TYPE_OUTPUT, input->ReportId, buffer, offset))
				return 0;

			buffer[1] = IAP_LCB_CONTINUATION;
			offset = 2;
		}

		payload -= fit;
	}

	for (int i = 0; i < payload; i++) 
		sum += buffer[offset++] = data[payload_offset++];
	buffer[offset++] = -sum;

	input = _get_best_bigger_report(iap, offset);
	if (input != NULL)
	{
		int report_size = ((input->Size * input->Count) >> 3) + 1;
		buffer[0] = input->ReportId;
		for (int i = offset; i < report_size; i++) 
			buffer[i] = 0; 
		
		if (usbd_hid_set_report(iap->Function, USB_HID_REPORT_TYPE_OUTPUT, input->ReportId, buffer, offset))
			return 1;
	}
	return 0;
}


