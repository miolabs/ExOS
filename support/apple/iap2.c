#include "iap2.h"
#include "iap2_fid.h"
#include "cp20.h"
#include <kernel/fifo.h>
#include <support/usb/driver/hid.h>

#define THREAD_STACK 1536
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

static int _match_device(USB_HOST_DEVICE *device);
static int _match_handler(HID_FUNCTION *func, HID_REPORT_INPUT *input);
static void _end_enum(HID_FUNCTION *func);
static void _notify(HID_FUNCTION *func, HID_REPORT_INPUT *input, unsigned char *data);
static const HID_REPORT_DRIVER _iap2_hidd = { 
	.MatchDevice = _match_device, .MatchInputHandler = _match_handler,
	.EndReportEnum = _end_enum,
	.Notify = _notify };
static HID_REPORT_MANAGER _hidd_manager = { .Driver = &_iap2_hidd };

static HID_FUNCTION *_hid_func = NULL;
#define IAP2_MAX_REPORT_INPUTS 4
static HID_REPORT_INPUT *_inputs[IAP2_MAX_REPORT_INPUTS];
static IAP2_PARSER_STATE _parser_state;
static int _inputs_count = 0;

static unsigned long _transaction = 1;

#define IAP2_MAX_PENDING_REQUESTS 3		// including incoming requests
static IAP2_REQUEST _requests[IAP2_MAX_PENDING_REQUESTS];
static EXOS_FIFO _free_requests_fifo;
static EXOS_FIFO _pending_incoming_fifo;
static EXOS_EVENT _incoming_event;
static EXOS_MUTEX _busy_requests_lock;
static EXOS_LIST _busy_requests_list;

static IAP2_CMD _incoming_cmd;
static IAP2_PAYLOAD_BUFFER _incoming_buffer;

void iap2_initialize()
{
	exos_fifo_create(&_free_requests_fifo, NULL);
	for(int i = 0; i < IAP2_MAX_PENDING_REQUESTS; i++)
	{
		IAP2_REQUEST *req = &_requests[i];
		exos_event_create(&req->CompletedEvent);
		exos_fifo_queue(&_free_requests_fifo, (EXOS_NODE *)req);
	}
	exos_event_create(&_incoming_event);
	exos_fifo_create(&_pending_incoming_fifo, &_incoming_event);

	exos_mutex_create(&_busy_requests_lock);
	list_initialize(&_busy_requests_list);

	apple_cp20_initialize();

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

	if (_inputs_count < IAP2_MAX_REPORT_INPUTS)
	{
		_inputs[_inputs_count++] = input;
		return 1;
	}
	return 0;
}

static void _end_enum(HID_FUNCTION *func)
{
	if (_hid_func == func)
	{
		// FIXME: check running state / support uninstalling
		exos_thread_create(&_thread, 0, _stack, THREAD_STACK, NULL, _service, func);
	}
}

static IAP2_REQUEST *_alloc_request()
{
	IAP2_REQUEST *req = (IAP2_REQUEST *)exos_fifo_dequeue(&_free_requests_fifo);
    if (req != NULL)
	{
		exos_event_reset(&req->CompletedEvent);
		req->Input = req->Output = NULL;
	}
	return req;
}

static void _free_request(IAP2_REQUEST *req)
{
	exos_fifo_queue(&_free_requests_fifo, (EXOS_NODE *)req);
}

static IAP2_REQUEST *_find_pending_request(unsigned short tr_id)
{
	IAP2_REQUEST *found = NULL;
	exos_mutex_lock(&_busy_requests_lock);
	FOREACH(node, &_busy_requests_list)
	{
		IAP2_REQUEST *req = (IAP2_REQUEST *)node;
		if (req->Output != NULL &&
			req->Output->Transaction == tr_id)
		{
			found = req;
			break;
		}
	}
	exos_mutex_unlock(&_busy_requests_lock);
	return found;
}

static void _parse(IAP2_REQUEST *req)
{
	int done = 0;
	IAP2_CMD *resp = req->Input;
	if (req->Output != NULL)
	{
		if (resp->LingoID == IAP2_LINGO_GENERAL &&
			resp->CommandID == IAP2_CMD_IPODACK)
		{
			if (resp->Data[1] == req->Output->CommandID)
			{
				req->ErrorCode = resp->Data[0];
				done = 1;
			}
		}
		else
		{
			req->ErrorCode = IAP2_OK;
			done = 1;
		}

		if (done)
		{
			exos_mutex_lock(&_busy_requests_lock);
			list_remove((EXOS_NODE *)req);
			exos_mutex_unlock(&_busy_requests_lock);
	
			exos_event_set(&req->CompletedEvent);
		}
	}
	else
	{
		exos_event_set(&req->CompletedEvent);
		exos_fifo_queue(&_pending_incoming_fifo, (EXOS_NODE *)req);
	}
}

static void _notify(HID_FUNCTION *func, HID_REPORT_INPUT *input, unsigned char *data)
{
	IAP2_PARSER_STATE *parser = &_parser_state;
	IAP2_REQUEST *req;
	int packet_size = ((input->Size * input->Count) >> 3);
	unsigned char lcb = data[0];

	int offset = 1;
	if (!(lcb & IAP2_LCB_CONTINUATION) &&
		data[offset++] == 0x55)
	{
		int length = data[offset++];
		if (length == 0) 
			length = (data[offset++] << 8) | data[offset++];

		unsigned char lingo = data[offset++];
		unsigned short cmd = (lingo == 4) ? data[offset++] << 8 : 0;
		cmd |= data[offset++];
		unsigned short tr_id = data[offset++] << 8;
		tr_id |= data[offset++];
		
		req = _find_pending_request(tr_id);
		if (req == NULL)
		{
			req = _alloc_request();
			if (req != NULL)
			{
				req->Input = &_incoming_cmd;
				_incoming_cmd.Data = _incoming_buffer;
				_incoming_cmd.Length = sizeof(_incoming_buffer);
			}
		}
		if (req != NULL)
		{
			IAP2_CMD *incoming = req->Input;
			incoming->Transaction = tr_id; 
			incoming->LingoID = lingo;
			incoming->CommandID = cmd;
			length -= (offset - 3);
			if (length < incoming->Length) incoming->Length = length;

			parser->Checksum = 0;
			for(int i = 2; i < offset; i++) parser->Checksum += data[i];
			parser->PayloadOffset = 0;
		}
		parser->Request = req;
	}
	else req = parser->Request;

	if (req != NULL)
	{
		IAP2_CMD *incoming = req->Input;
		int payload_length = packet_size - offset;
		int remaining = incoming->Length - parser->PayloadOffset;
		if (remaining < payload_length) payload_length = remaining;
		for (int i = 0; i < payload_length; i++) 
			parser->Checksum += incoming->Data[parser->PayloadOffset++] = data[offset++];
		
		if (!(lcb & IAP2_LCB_MORE_TO_FOLLOW) &&
			parser->PayloadOffset == incoming->Length && 
			offset < packet_size &&
			data[offset++] == (0x100 - parser->Checksum))
		{
			_parse(req);
		}
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

static int _send_cmd(IAP2_CMD *cmd)
{
	HID_REPORT_INPUT *input;
	unsigned char buffer[64];
	unsigned char sum = 0;
	
	buffer[1] = IAP2_LCB_START;
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
	int payload = cmd->Data != NULL ? cmd->Length : 0;
	while(payload > 0)
	{
		int fit = sizeof(buffer) - offset;
		if (fit > payload) fit = payload;
		input = _get_best_smaller_report(offset + fit);
		if (input == NULL) break;

		int report_size = ((input->Size * input->Count) >> 3) + 1;
		if ((offset + fit) > report_size) fit = report_size - offset;

		for (int i = 0; i < fit; i++) 
			sum += buffer[offset++] = cmd->Data[payload_offset++];

		if (offset == report_size)
		{
			buffer[0] = input->ReportId;
			buffer[1] |= IAP2_LCB_MORE_TO_FOLLOW;
			if (!usbd_hid_set_report(_hid_func, USB_HID_REPORT_TYPE_OUTPUT, input->ReportId, buffer, offset))
				break;
			buffer[1] = IAP2_LCB_CONTINUATION;
			offset = 2;
		}

		payload -= fit;
	}

	for (int i = 0; i < payload; i++) 
		sum += buffer[offset++] = cmd->Data[payload_offset++];
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

IAP2_REQUEST *iap2_begin_req(IAP2_CMD *cmd, IAP2_CMD *resp)
{
	IAP2_REQUEST *req = _alloc_request();
	if (req != NULL)
	{
		cmd->Transaction = _transaction++;
		req->Output = cmd;
		req->Input = resp;

		exos_mutex_lock(&_busy_requests_lock);
		list_add_tail(&_busy_requests_list, (EXOS_NODE *)req);
		exos_mutex_unlock(&_busy_requests_lock);

		if (_send_cmd(cmd))
			return req;

		// TODO: dispose req
	}
	return NULL;
}

IAP2_CMD_STATUS iap2_end_req(IAP2_REQUEST *req, int timeout)
{
	int error = exos_event_wait(&req->CompletedEvent, timeout);
	IAP2_CMD_STATUS status = error ? IAP2_ERROR_OPERATION_TIMED_OUT : req->ErrorCode;
	_free_request(req);
	return status;
}

IAP2_CMD_STATUS iap2_do_req(IAP2_CMD *cmd, IAP2_CMD *resp)
{
	IAP2_REQUEST *req = iap2_begin_req(cmd, resp);
	return (req != NULL) ? iap2_end_req(req, 1000) : IAP2_ERROR_COMMAND_FAILED;
}

IAP2_CMD_STATUS iap2_do_req2(IAP2_CMD_ID cmd_id, IAP2_CMD *resp)
{
	IAP2_CMD cmd = (IAP2_CMD) { .CommandID = cmd_id };
	return iap2_do_req(&cmd, resp);
}

IAP2_CMD_STATUS iap2_do_req3(IAP2_CMD_ID cmd_id, unsigned char *cmd_data, int cmd_length, IAP2_CMD *resp)
{
	IAP2_CMD cmd = (IAP2_CMD) { .CommandID = cmd_id, .Data = cmd_data, .Length = cmd_length };
	return iap2_do_req(&cmd, resp);
}

int iap2_get_incoming_cmd(IAP2_CMD *cmd, int timeout)
{
	IAP2_REQUEST *req = (IAP2_REQUEST *)exos_fifo_wait(&_pending_incoming_fifo, timeout);
	if (req)
	{
		IAP2_CMD *input = req->Input;
		cmd->LingoID = input->LingoID;
		cmd->CommandID = input->CommandID;
		if (cmd->Data)
		{
			int length = input->Length < cmd->Length ? input->Length : cmd->Length;
			for(int i = 0; i < length; i++) cmd->Data[i] = input->Data[i];
			cmd->Length = length;
		}

		_free_request(req);
		return 1;
	}
	return 0;
}

static int _identify()
{
	IAP2_CMD_STATUS status;
	unsigned char buffer[256];
	IAP2_CMD resp = (IAP2_CMD) { .Data = buffer, .Length = sizeof(buffer) };
	status = iap2_do_req2(IAP2_CMD_START_IDPS, &resp);
    if (status == IAP2_OK)
	{
		resp = (IAP2_CMD) { .Data = buffer, .Length = sizeof(buffer) };
		status = iap2_do_req2(IAP2_CMD_REQUEST_TRANSPORT_MAX_PAYLOAD_SIZE, &resp);
		int max_payload = (status == IAP2_OK && resp.CommandID == IAP2_CMD_RETURN_TRANSPORT_MAX_PAYLOAD_SIZE) ? 
			buffer[0] << 8 | buffer[1] : 
			(status == IAP2_ERROR_BAD_PARAMETER ? IAP2_MAX_PACKET_BUFFER : 0);
		if (max_payload != 0)
		{
			int fid_data = iap2_fid_fill(buffer, sizeof(buffer));
			resp = (IAP2_CMD) { .Data = buffer + fid_data, .Length = sizeof(buffer) };
			status = iap2_do_req3(IAP2_CMD_SET_FID_TOKEN_VALUES, buffer, fid_data, &resp);
			if (status == IAP2_OK)
			{
				unsigned char end_param = 0; // immediately request authentication
				resp = (IAP2_CMD) { .Data = buffer, .Length = sizeof(buffer) };
				status = iap2_do_req3(IAP2_CMD_END_IDPS, &end_param, 1, &resp);
				if (status == IAP2_OK)
					return 1;
			}
		}
	}
	return 0;
}

static int _read_cert(unsigned char *ppart, unsigned char *buffer, unsigned short total_length)
{
	unsigned char parts_done = *ppart;
	int length = total_length - (parts_done * 128);
	if (length > 128) length = 128;
	int offset = 0;
	buffer[offset++] = 0x02;	// auth major version
	buffer[offset++] = 0x00;
	buffer[offset++] = parts_done;
	buffer[offset++] = ((total_length + 127) >> 7) - 1;
	int done = apple_cp2_read_acc_cert_page(parts_done++, buffer + offset, length);
	*ppart = parts_done;
	return done ? offset + length : 0;
}

static int _authenticate(IAP2_USB_FUNCTION *func)
{
	IAP2_CMD cmd, resp;
	unsigned char cmd_buffer[256];
	unsigned char resp_buffer[256];
	unsigned short total_length;
	unsigned char parts_done;
	IAP2_CMD_ID cmd_ack;
	int offset;
	while(1)
	{
		cmd = (IAP2_CMD) { .Data = cmd_buffer, .Length = sizeof(cmd_buffer) };
		if (iap2_get_incoming_cmd(&cmd, 1000))
		{
			offset = 0;
			if (cmd.LingoID == IAP2_LINGO_GENERAL)
			{
				switch(cmd.CommandID)
				{
					case IAP2_CMD_IPODACK:
						if (cmd.Data[0] == IAP2_OK)
						{
							cmd_ack = cmd.Data[1];
							switch(cmd_ack)
							{
								case IAP2_CMD_RET_ACC_AUTH_INFO:
									offset = _read_cert(&parts_done, resp_buffer, total_length);
									resp = (IAP2_CMD) { .CommandID = IAP2_CMD_RET_ACC_AUTH_INFO, .Data = resp_buffer, .Length = offset, .Transaction = cmd.Transaction };
									_send_cmd(&resp);
									break;
							}
						}
						break;
					case IAP2_CMD_GET_ACC_AUTH_INFO:
						if (apple_cp2_read_acc_cert_length(&total_length))
						{
							parts_done = 0;
							offset = _read_cert(&parts_done, resp_buffer, total_length);
							resp = (IAP2_CMD) { .CommandID = IAP2_CMD_RET_ACC_AUTH_INFO, .Data = resp_buffer, .Length = offset, .Transaction = cmd.Transaction };
                            _send_cmd(&resp);
						}
						break;
					case IAP2_CMD_ACK_ACC_AUTH_INFO:
						if (cmd.Data[0] == 0)	// 0x00 = auth info supported
						{
							// hmmmm, nothing to do but wait next command							 
						}
						break;
					case IAP2_CMD_GET_ACC_AUTH_SIGNATURE:
						if (cmd.Length == 21)
						{
							offset = apple_cp2_get_auth_signature(cmd.Data, cmd.Length - 1, resp_buffer);
							resp = (IAP2_CMD) { .CommandID = IAP2_CMD_RET_ACC_AUTH_SIGNATURE, .Data = resp_buffer, .Length = offset, .Transaction = cmd.Transaction };
							_send_cmd(&resp);
						}
						break;
					case IAP2_CMD_ACK_ACC_AUTH_STATUS:
						if (cmd.Length == 1 && 
							cmd.Data[0] == 0) // auth operation passed
						{
							// TODO: notify app level that lingo ids are ready to be used
						}
						break;
				}
			}
		}
	}
	return 0;
}

static void *_service(void *arg)
{
	IAP2_USB_FUNCTION *func = (IAP2_USB_FUNCTION *)arg;

	if (_identify())
	{
		_authenticate(func);
	}
}



