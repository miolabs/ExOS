#include "iap.h"
#include "iap_core.h"
#include <support/usb/driver/hid.h>

static int _match_device(USB_HOST_DEVICE *device);
static int _match_handler(HID_FUNCTION *func, HID_REPORT_INPUT *input);
static void _start(HID_FUNCTION *func);
static void _stop(HID_FUNCTION *func);
static void _notify(HID_FUNCTION *func, HID_REPORT_INPUT *input, unsigned char *data);
static const HID_REPORT_DRIVER _iap_hidd = { 
	.MatchDevice = _match_device, .MatchInputHandler = _match_handler,
	.Start = _start, .Stop = _stop,
	.Notify = _notify };
static HID_REPORT_MANAGER _hidd_manager = { .Driver = &_iap_hidd };

static HID_FUNCTION *_hid_func = NULL;
#define IAP_MAX_REPORT_INPUTS 4
static HID_REPORT_INPUT *_inputs[IAP_MAX_REPORT_INPUTS];
static int _inputs_count = 0;

#define IAP_MAX_INPUT_BUFFER 512
static unsigned long _iap_offset = 0;
static unsigned char _iap_buffer[IAP_MAX_INPUT_BUFFER];

void iap_initialize()
{
	iap_core_initialize();

	usbd_hid_add_manager(&_hidd_manager);
}

static int _match_device(USB_HOST_DEVICE *device)
{
	return device->Vendor == 0x05ac // Apple
		&& (device->Product & 0xff00) == 0x1200; // iAP2 apple device
}

static int _match_handler(HID_FUNCTION *func, HID_REPORT_INPUT *input)
{
	if (_hid_func == NULL) _hid_func = func;
	else if (_hid_func != func)  return 0;

	if (_inputs_count < IAP_MAX_REPORT_INPUTS)
	{
		_inputs[_inputs_count++] = input;
		return 1;
	}
	return 0;
}

static void _start(HID_FUNCTION *func)
{
	if (_hid_func == func)
	{
		// FIXME: check running state / support uninstalling
		iap_core_start();
	}
}

static void _stop(HID_FUNCTION *func)
{
	if (_hid_func == func)
	{
		iap_core_stop();
		_inputs_count = 0;
	}
}

static void _notify(HID_FUNCTION *func, HID_REPORT_INPUT *input, unsigned char *data)
{
	int packet_size = ((input->Size * input->Count) >> 3);
	int offset = 0;
	unsigned char lcb = data[offset++];

	if (!(lcb & IAP_LCB_CONTINUATION))
		_iap_offset = 0;
	
	int fit = packet_size - 1;
	if ((_iap_offset + fit) > sizeof(_iap_buffer))
		fit = sizeof(_iap_buffer) - _iap_offset;
	for (int i = 0; i < fit; i++) _iap_buffer[_iap_offset++] = data[offset++];

	if (!(lcb & IAP_LCB_MORE_TO_FOLLOW))
	{
		iap_core_parse(_iap_buffer, _iap_offset);
		_iap_offset = sizeof(_iap_buffer);
	}
}


static HID_REPORT_INPUT *_get_best_smaller_report(int size)
{
	HID_REPORT_INPUT *best = NULL;
	int best_size = 0;
	for(int i = 0; i < _inputs_count; i++)
	{
		HID_REPORT_INPUT *input = _inputs[i];
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

static HID_REPORT_INPUT *_get_best_bigger_report(int size)
{
	HID_REPORT_INPUT *best = NULL;
	int best_size = 65;
	for(int i = 0; i < _inputs_count; i++)
	{
		HID_REPORT_INPUT *input = _inputs[i];
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
		input = _get_best_smaller_report(offset + fit);
		if (input == NULL) break;

		int report_size = ((input->Size * input->Count) >> 3) + 1;
		if ((offset + fit) > report_size) fit = report_size - offset;

		for (int i = 0; i < fit; i++) 
			sum += buffer[offset++] = data[payload_offset++];

		if (offset == report_size)
		{
			buffer[0] = input->ReportId;
			buffer[1] |= IAP_LCB_MORE_TO_FOLLOW;
			if (!usbd_hid_set_report(_hid_func, USB_HID_REPORT_TYPE_OUTPUT, input->ReportId, buffer, offset))
				return 0;

			buffer[1] = IAP_LCB_CONTINUATION;
			offset = 2;
		}

		payload -= fit;
	}

	for (int i = 0; i < payload; i++) 
		sum += buffer[offset++] = data[payload_offset++];
	buffer[offset++] = -sum;

	input = _get_best_bigger_report(offset);
	if (input != NULL)
	{
		int report_size = ((input->Size * input->Count) >> 3) + 1;
		buffer[0] = input->ReportId;
		for (int i = offset; i < report_size; i++) 
			buffer[i] = 0; 
		
		if (usbd_hid_set_report(_hid_func, USB_HID_REPORT_TYPE_OUTPUT, input->ReportId, buffer, offset))
			return 1;
	}
	return 0;
}


