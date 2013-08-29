#include "iap_core.h"
#include "iap_fid.h"
#include "cp20.h"
#include "iap_comm.h"
#include <kernel/fifo.h>
#include <kernel/panic.h>
#include <support/board_hal.h>

#define THREAD_STACK 1536
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);

static unsigned long _transaction = 1;

#define IAP_MAX_PENDING_REQUESTS 2
static IAP_REQUEST _requests[IAP_MAX_PENDING_REQUESTS];
static EXOS_FIFO _free_requests_fifo;
static EXOS_MUTEX _busy_requests_lock;
static EXOS_LIST _busy_requests_list;

#define IAP_MAX_INCOMING_COMMANDS 3
static IAP_CMD_NODE _incoming_cmds[IAP_MAX_INCOMING_COMMANDS];
static EXOS_FIFO _free_cmds_fifo;
static EXOS_FIFO _incoming_cmds_fifo;
static EXOS_EVENT _incoming_cmds_event;

void iap_core_initialize()
{
	exos_fifo_create(&_free_requests_fifo, NULL);
	for(int i = 0; i < IAP_MAX_PENDING_REQUESTS; i++)
	{
		IAP_REQUEST *req = &_requests[i];
		exos_event_create(&req->CompletedEvent);
		exos_fifo_queue(&_free_requests_fifo, (EXOS_NODE *)req);
	}
	exos_mutex_create(&_busy_requests_lock);
	list_initialize(&_busy_requests_list);

	exos_fifo_create(&_free_cmds_fifo, NULL);
	for(int i = 0; i < IAP_MAX_INCOMING_COMMANDS; i++)
	{
		IAP_CMD_NODE *cmd_node = &_incoming_cmds[i];
		exos_fifo_queue(&_free_cmds_fifo, (EXOS_NODE *)cmd_node);
	}
	exos_event_create(&_incoming_cmds_event);
	exos_fifo_create(&_incoming_cmds_fifo, &_incoming_cmds_event);

	apple_cp20_initialize();

   	iap_comm_initialize();
}

void iap_core_start()
{
	// FIXME 
	exos_thread_create(&_thread, 1, _stack, THREAD_STACK, NULL, _service, NULL);
}

#ifdef DEBUG
static void _warning()
{
	hal_led_set(0, 1);
}
#endif

static IAP_CMD_NODE *_alloc_cmd()
{
	IAP_CMD_NODE *cmd_node = (IAP_CMD_NODE *)exos_fifo_dequeue(&_free_cmds_fifo);
	if (cmd_node)
	{
		*cmd_node = (IAP_CMD_NODE) { .Cmd.Length = sizeof(cmd_node->Buffer) };
	}
    return cmd_node;
}

static void _free_cmd(IAP_CMD_NODE *cmd_node)
{
	exos_fifo_queue(&_free_cmds_fifo, (EXOS_NODE *)cmd_node);
}

static IAP_REQUEST *_alloc_request()
{
	IAP_REQUEST *req = (IAP_REQUEST *)exos_fifo_dequeue(&_free_requests_fifo);
    if (req != NULL)
	{
		exos_event_reset(&req->CompletedEvent);
		req->Cmd = req->Response = NULL;
	}
	return req;
}

static void _free_request(IAP_REQUEST *req)
{
	exos_fifo_queue(&_free_requests_fifo, (EXOS_NODE *)req);
}

static IAP_REQUEST *_find_pending_request_by_tr(unsigned short tr_id)
{
	IAP_REQUEST *found = NULL;
	exos_mutex_lock(&_busy_requests_lock);
	FOREACH(node, &_busy_requests_list)
	{
		IAP_REQUEST *req = (IAP_REQUEST *)node;
		if (req->Cmd != NULL && req->Cmd->Transaction == tr_id)
		{
			found = req;
			break;
		}
	}
	exos_mutex_unlock(&_busy_requests_lock);
	return found;
}

static IAP_REQUEST *_find_pending_request_by_cmd(unsigned short cmd_id)
{
	IAP_REQUEST *found = NULL;
	exos_mutex_lock(&_busy_requests_lock);
	FOREACH(node, &_busy_requests_list)
	{
		IAP_REQUEST *req = (IAP_REQUEST *)node;
		if (req->Cmd != NULL && req->Cmd->CommandID == cmd_id)
		{
			found = req;
			break;
		}
	}
	exos_mutex_unlock(&_busy_requests_lock);
	return found;
}

void iap_core_parse(unsigned char *data, int length)
{
	IAP_CMD *cmd;
	IAP_REQUEST *req;
	IAP_CMD_NODE *cmd_node;
    unsigned short cmd_length;
	unsigned char checksum;
	unsigned char lingo;
	unsigned short cmd_id;
	unsigned short tr_id;
	IAP_CMD_STATUS error_code;
	
	int offset_start;
	int offset = 0; 
	while(offset < length)
	{
		if (data[offset++] != 0x55)
			break;

		cmd_length = data[offset++];
		if (cmd_length == 0)
		{
			cmd_length = (data[offset++] << 8);
			cmd_length |= data[offset++];
		}
		checksum = cmd_length + (cmd_length >> 8);
		offset_start = offset;

		lingo = data[offset++];
		checksum += lingo;
		cmd_id = data[offset++];
		checksum += cmd_id;
		if (lingo == 4)
		{
			cmd_id = (cmd_id << 8) | data[offset++];
			checksum += cmd_id;
		}

		req = NULL;
		cmd_node = NULL;
		tr_id = 0;
		error_code = IAP_OK;

		if (lingo == IAP_LINGO_GENERAL && 
			cmd_id == IAP_CMD_IPODACK)
		{
			if (cmd_length == 6 || cmd_length == 10 || cmd_length == 12)
			{
				tr_id = (data[offset++] << 8);
				tr_id |= data[offset++];
				req = _find_pending_request_by_tr(tr_id);
			}

			error_code = data[offset];
			if (req == NULL) 
				req = _find_pending_request_by_cmd(data[offset + 1]);
		}
		else
		{
       		tr_id = (data[offset++] << 8);
			tr_id |= data[offset++];

			req = _find_pending_request_by_tr(tr_id);
		}
		checksum += tr_id + (tr_id >> 8);

		cmd = NULL;
		unsigned char *payload;
		if (req != NULL)
		{
			cmd = req->Response;
			payload = req->ResponseData;
		}
		else
		{
			cmd_node = _alloc_cmd();
			if (cmd_node != NULL) 
			{
				cmd = &cmd_node->Cmd;
				payload = cmd_node->Buffer;
			}
		}

		if (cmd != NULL)
		{
			int payload_length = cmd_length - (offset - offset_start);

			cmd->LingoID = lingo;
			cmd->ErrorCode = error_code;
			cmd->Transaction = tr_id;
			cmd->CommandID = cmd_id;
			if (payload_length < cmd->Length)
				cmd->Length = payload_length;

			int copy_length = payload != NULL ? cmd->Length : 0;
			for (int i = 0; i < copy_length; i++) checksum += payload[i] = data[offset++];
			for (int i = copy_length; i < payload_length; i++) checksum += data[offset++];
		}
		else 
		{
			//_warning();
			break;
		}

		if (data[offset++] == (unsigned char)-checksum)
		{
			if (req != NULL)
			{
				exos_mutex_lock(&_busy_requests_lock);
#ifdef DEBUG
				if (!list_find_node(&_busy_requests_list, (EXOS_NODE *)req))
					_warning();
#endif
				list_remove((EXOS_NODE *)req);
				exos_mutex_unlock(&_busy_requests_lock);
				exos_event_set(&req->CompletedEvent);		
			}
			else if (cmd_node != NULL)
			{
				exos_fifo_queue(&_incoming_cmds_fifo, (EXOS_NODE *)cmd_node);
			}
#ifdef DEBUG
			else _warning();
#endif
		}
		else 
		{
			if (cmd_node != NULL)
			{
				_free_cmd(cmd_node);
			}
			break;
		}
	}
}


IAP_REQUEST *iap_begin_req(IAP_CMD *cmd, unsigned char *cmd_data, IAP_CMD *resp, unsigned char *resp_data)
{
	IAP_REQUEST *req = _alloc_request();
	if (req != NULL)
	{
		cmd->Transaction = _transaction++;
		req->Cmd = cmd;
		req->CmdData = cmd_data;
		req->Response = resp;
		req->ResponseData = resp_data;

		exos_mutex_lock(&_busy_requests_lock);
		list_add_tail(&_busy_requests_list, (EXOS_NODE *)req);
		exos_mutex_unlock(&_busy_requests_lock);

		if (iap_send_cmd(cmd, cmd_data))
			return req;

		exos_mutex_lock(&_busy_requests_lock);
		list_remove((EXOS_NODE *)req);
		exos_mutex_unlock(&_busy_requests_lock);

		_free_request(req);
	}
#ifdef DEBUG
	_warning();
#endif
	return NULL;
}

IAP_CMD_STATUS iap_end_req(IAP_REQUEST *req, int timeout)
{
	if (req == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	int error = exos_event_wait(&req->CompletedEvent, timeout);
	IAP_CMD_STATUS status = error ? IAP_ERROR_OPERATION_TIMED_OUT : req->Response->ErrorCode;
	_free_request(req);
	return status;
}

IAP_CMD_STATUS iap_do_req(IAP_CMD *cmd, unsigned char *cmd_data, IAP_CMD *resp, unsigned char *resp_data)
{
	IAP_REQUEST *req = iap_begin_req(cmd, cmd_data, resp, resp_data);
	return (req != NULL) ? iap_end_req(req, 1000) : IAP_ERROR_COMMAND_FAILED;
}

IAP_CMD_STATUS iap_do_req2(IAP_CMD_ID cmd_id, IAP_CMD *resp, unsigned char *resp_data)
{
	IAP_CMD cmd = (IAP_CMD) { .CommandID = cmd_id };
	return iap_do_req(&cmd, NULL, resp, resp_data);
}

IAP_CMD_STATUS iap_do_req3(IAP_CMD_ID cmd_id, unsigned char *cmd_data, int cmd_length, IAP_CMD *resp, unsigned char *resp_data)
{
	IAP_CMD cmd = (IAP_CMD) { .CommandID = cmd_id, .Length = cmd_length };
	return iap_do_req(&cmd, cmd_data, resp, resp_data);
}

int iap_get_incoming_cmd(IAP_CMD *cmd, unsigned char *cmd_data, int timeout)
{
	IAP_CMD_NODE *cmd_node = (IAP_CMD_NODE *)exos_fifo_wait(&_incoming_cmds_fifo, timeout);
	if (cmd_node)
	{
		IAP_CMD *input = &cmd_node->Cmd;
		cmd->LingoID = input->LingoID;
		cmd->ErrorCode = input->ErrorCode;
		cmd->Transaction = input->Transaction;
		cmd->CommandID = input->CommandID;

		int length = input->Length < cmd->Length ? input->Length : cmd->Length;
		for(int i = 0; i < length; i++) cmd_data[i] = cmd_node->Buffer[i];
		cmd->Length = length;

		_free_cmd(cmd_node);
		return 1;
	}
	return 0;
}

static int _identify()
{
	IAP_CMD_STATUS status;
	unsigned char buffer[256];
	IAP_CMD resp = (IAP_CMD) { .Length = sizeof(buffer) };
	status = iap_do_req2(IAP_CMD_START_IDPS, &resp, buffer);
    if (status == IAP_OK)
	{
		resp = (IAP_CMD) { .Length = sizeof(buffer) };
		status = iap_do_req2(IAP_CMD_REQUEST_TRANSPORT_MAX_PAYLOAD_SIZE, &resp, buffer);
		int max_payload = (status == IAP_OK && resp.CommandID == IAP_CMD_RETURN_TRANSPORT_MAX_PAYLOAD_SIZE) ? 
			buffer[0] << 8 | buffer[1] : 
			(status == IAP_ERROR_BAD_PARAMETER ? IAP_MAX_PACKET_BUFFER : 0);
		if (max_payload != 0)
		{
			int fid_data = iap_fid_fill(buffer, sizeof(buffer));
			resp = (IAP_CMD) { .Length = sizeof(buffer) - fid_data };
			status = iap_do_req3(IAP_CMD_SET_FID_TOKEN_VALUES, buffer, fid_data, &resp, buffer + fid_data);
			if (status == IAP_OK)
			{
				unsigned char end_param = 0; // immediately request authentication
				resp = (IAP_CMD) { .Length = sizeof(buffer) };
				status = iap_do_req3(IAP_CMD_END_IDPS, &end_param, 1, &resp, buffer);
				if (status == IAP_OK)
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
	if (length > 0)
	{
		if (length > 128) length = 128;
		int offset = 0;
		buffer[offset++] = 0x02;	// auth major version
		buffer[offset++] = 0x00;
		buffer[offset++] = parts_done;
		buffer[offset++] = ((total_length + 127) >> 7) - 1;
		if (apple_cp2_read_acc_cert_page(parts_done, buffer + offset, length))
		{
			*ppart = ++parts_done;
			offset += length;
			return offset;
		}
	}
	return 0;
}

static int _slave_io()
{
	IAP_CMD cmd, resp;
	unsigned char cmd_buffer[256];
	unsigned char resp_buffer[256];
	unsigned short total_length;
	unsigned char parts_done;
	int offset;
	int done;
#ifdef DEBUG
	int auth_started = 0;
	int auth_done = 0;
#endif
	while(1)
	{
		cmd = (IAP_CMD) { .Length = sizeof(cmd_buffer) };
		if (iap_get_incoming_cmd(&cmd, cmd_buffer, 1000))
		{
			offset = 0;
			if (cmd.LingoID == IAP_LINGO_GENERAL)
			{
				switch(cmd.CommandID)
				{
					case IAP_CMD_IPODACK:
						if (cmd.ErrorCode == IAP_OK)
						{
							switch(cmd_buffer[1])
							{
								case IAP_CMD_RET_ACC_AUTH_INFO:
									offset = _read_cert(&parts_done, resp_buffer, total_length);
									resp = (IAP_CMD) { .CommandID = IAP_CMD_RET_ACC_AUTH_INFO, .Length = offset, .Transaction = cmd.Transaction };
									iap_send_cmd(&resp, resp_buffer);
									break;
								case IAP_CMD_ACCESORY_DATA_TRANSFER:
									// nothing to do here
									break;
#ifdef DEBUG
								default:
									_warning();
									break;
#endif
							}
						}
#ifdef DEBUG
						else _warning();
#endif
						break;
					case IAP_CMD_GET_ACC_AUTH_INFO:
						if (apple_cp2_read_acc_cert_length(&total_length))
						{
							parts_done = 0;
							offset = _read_cert(&parts_done, resp_buffer, total_length);
							resp = (IAP_CMD) { .CommandID = IAP_CMD_RET_ACC_AUTH_INFO, .Length = offset, .Transaction = cmd.Transaction };
                            iap_send_cmd(&resp, resp_buffer);
						}
						break;
					case IAP_CMD_ACK_ACC_AUTH_INFO:
						if (cmd_buffer[0] == 0)	// 0x00 = auth info supported
						{
							// hmmmm, nothing to do but wait next command							 
						}
						break;
					case IAP_CMD_GET_ACC_AUTH_SIGNATURE:
						if (cmd.Length == 21)
						{
#ifdef DEBUG
							auth_started++;
#endif
							offset = apple_cp2_get_auth_signature(cmd_buffer, cmd.Length - 1, resp_buffer);
							resp = (IAP_CMD) { .CommandID = IAP_CMD_RET_ACC_AUTH_SIGNATURE, .Length = offset, .Transaction = cmd.Transaction };
							iap_send_cmd(&resp, resp_buffer);
						}
						break;
					case IAP_CMD_ACK_ACC_AUTH_STATUS:
						if (cmd.Length == 1 && 
							cmd_buffer[0] == 0) // auth operation passed
						{
#ifdef DEBUG
							auth_done++;
#endif
							// TODO: notify app level that lingo ids are ready to be used
						}
#ifdef DEBUG
						else _warning();
#endif
						break;
					case IAP_CMD_OPEN_DATA_SESSION_FOR_PROTOCOL:
						// FIXME: we are ignoring protocol field
						done = iap_open_session((cmd_buffer[0] << 8) | cmd_buffer[1], cmd_buffer[2]);
						resp_buffer[offset++] = done ? IAP_OK : IAP_ERROR_BAD_PARAMETER;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (IAP_CMD) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = cmd.Transaction };
                        iap_send_cmd(&resp, resp_buffer);
						break;
					case IAP_CMD_IPOD_DATA_TRANSFER:
						if (cmd.Length > 2)
						{
							iap_comm_write((cmd_buffer[0] << 8) | cmd_buffer[1], &cmd_buffer[2], cmd.Length - 2);
						}
						resp_buffer[offset++] = IAP_OK;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (IAP_CMD) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = cmd.Transaction };
                        iap_send_cmd(&resp, resp_buffer);
#ifdef DEBUG		
//						resp = (IAP_CMD) { .CommandID = IAP_CMD_ACCESORY_DATA_TRANSFER, .Length = cmd.Length, .Transaction = _transaction++ };
//						iap_send_cmd(&resp, cmd_buffer);
#endif
						break;
					case IAP_CMD_CLOSE_DATA_SESSION:
						iap_close_session((cmd_buffer[0] << 8) | cmd_buffer[1]);
						resp_buffer[offset++] = IAP_OK;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (IAP_CMD) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = _transaction++ };
                        iap_send_cmd(&resp, resp_buffer);
						break;
#ifdef DEBUG
					default:
						_warning();
						break;
#endif
				}
			}
		}
	}
	return 0;
}

static void *_service(void *arg)
{
	if (_identify())
	{
		_slave_io();
	}
}




