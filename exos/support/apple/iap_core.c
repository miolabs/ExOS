#include "iap_core.h"
#include "iap_fid.h"
#include "cp20.h"
#include "iap_comm.h"
#include <kernel/fifo.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#ifdef IAP_DEBUG
#define _verbose(level, ...) verbose(level, "iAP-core", __VA_ARGS__)
#else
#define _verbose(level, ...) { /* nothing */ }
#endif


#define THREAD_STACK 2048
static exos_thread_t _thread;
static unsigned char _stack[THREAD_STACK] __attribute__((aligned(16)));
static void *_service(void *arg);
static int _service_busy = 0;
static int _service_exit = 0;

static unsigned long _transaction = 1;

#define IAP_MAX_PENDING_REQUESTS 2
static iap_request_t _requests[IAP_MAX_PENDING_REQUESTS];
static EXOS_FIFO _free_requests_fifo;
static mutex_t _busy_requests_lock;
static list_t _busy_requests_list;

#define IAP_MAX_INCOMING_COMMANDS 3
static iap_cmd_node_t _incoming_cmds[IAP_MAX_INCOMING_COMMANDS];
static EXOS_FIFO _free_cmds_fifo;
static EXOS_FIFO _incoming_cmds_fifo;
static event_t _incoming_cmds_event;

void iap_core_initialize()
{
	exos_fifo_create(&_free_requests_fifo, NULL);
	for(unsigned i = 0; i < IAP_MAX_PENDING_REQUESTS; i++)
	{
		iap_request_t *req = &_requests[i];
		exos_event_create(&req->CompletedEvent, EXOS_EVENTF_NONE);
		exos_fifo_queue(&_free_requests_fifo, &req->Node);
	}
	exos_mutex_create(&_busy_requests_lock);
	list_initialize(&_busy_requests_list);

	exos_fifo_create(&_free_cmds_fifo, NULL);
	for(unsigned i = 0; i < IAP_MAX_INCOMING_COMMANDS; i++)
	{
		iap_cmd_node_t *cmd_node = &_incoming_cmds[i];
		exos_fifo_queue(&_free_cmds_fifo, &cmd_node->Node);
	}
	exos_event_create(&_incoming_cmds_event, EXOS_EVENTF_NONE);
	exos_fifo_create(&_incoming_cmds_fifo, &_incoming_cmds_event);

	apple_cp20_initialize();

   	iap_comm_initialize();
}

void iap_core_start()
{
	if (!_service_busy)
	{
		_service_busy = 1;
		exos_thread_create(&_thread, 5, _stack, THREAD_STACK, _service, NULL);
	}
}

void iap_core_stop()
{
	_service_exit = 1;
	exos_thread_join(&_thread);
	_service_busy = 0;
}

#ifdef DEBUG
static void _die()
{
	kernel_panic(KERNEL_ERROR_UNKNOWN);
}
#endif

static iap_cmd_node_t *_alloc_cmd()
{
	iap_cmd_node_t *cmd_node = (iap_cmd_node_t *)exos_fifo_dequeue(&_free_cmds_fifo);
	if (cmd_node)
	{
		*cmd_node = (iap_cmd_node_t) { .Cmd.Length = sizeof(cmd_node->Buffer) };
	}
    return cmd_node;
}

static void _free_cmd(iap_cmd_node_t *cmd_node)
{
	exos_fifo_queue(&_free_cmds_fifo, &cmd_node->Node);
}

static iap_request_t *_alloc_request()
{
	iap_request_t *req = (iap_request_t *)exos_fifo_dequeue(&_free_requests_fifo);
    if (req != NULL)
	{
		exos_event_reset(&req->CompletedEvent);
		req->Cmd = req->Response = NULL;
	}
	return req;
}

static void _free_request(iap_request_t *req)
{
	exos_fifo_queue(&_free_requests_fifo, &req->Node);
}

static iap_request_t *_find_pending_request_by_tr(unsigned short tr_id)
{
	iap_request_t *found = NULL;
	exos_mutex_lock(&_busy_requests_lock);
	FOREACH(node, &_busy_requests_list)
	{
		iap_request_t *req = (iap_request_t *)node;
		if (req->Cmd != NULL && req->Cmd->Transaction == tr_id)
		{
			found = req;
			break;
		}
	}
	exos_mutex_unlock(&_busy_requests_lock);
	return found;
}

static iap_request_t *_find_pending_request_by_cmd(unsigned short cmd_id)
{
	iap_request_t *found = NULL;
	exos_mutex_lock(&_busy_requests_lock);
	FOREACH(node, &_busy_requests_list)
	{
		iap_request_t *req = (iap_request_t *)node;
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
	iap_cmd_t *cmd;
	iap_request_t *req;
	iap_cmd_node_t *cmd_node;
    unsigned short cmd_length;
	unsigned char checksum;
	unsigned char lingo;
	unsigned short cmd_id;
	unsigned short tr_id;
	iap_cmd_status_t error_code;
	
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
			{
				cmd->Length = payload_length;
			}
			int copy_length = payload != NULL ? cmd->Length : 0;
			for (int i = 0; i < copy_length; i++) checksum += payload[i] = data[offset++];
			for (int i = copy_length; i < payload_length; i++) checksum += data[offset++];
		}
		else 
		{
			_verbose(VERBOSE_ERROR, "incoming cmd discarded (could not be allocated)");
			break;
		}

		if (data[offset++] == (unsigned char)-checksum)
		{
			if (req != NULL)
			{
				exos_mutex_lock(&_busy_requests_lock);
#ifdef DEBUG
				if (!list_find_node(&_busy_requests_list, &req->Node))
					_die();	// matching request is not pending
#endif
				list_remove(&req->Node);
				exos_mutex_unlock(&_busy_requests_lock);

				//_verbose(VERBOSE_DEBUG, "-> CMD%02x(%x) tr=%x, iap request completed (CMD%02x(%x) tr=%x)",
				//	cmd->CommandID, cmd->LingoID, cmd->Transaction,
				//	req->Cmd->CommandID, req->Cmd->LingoID, req->Cmd->Transaction);

				exos_event_set(&req->CompletedEvent);
			}
			else if (cmd_node != NULL)
			{
				exos_fifo_queue(&_incoming_cmds_fifo, &cmd_node->Node);
				//_verbose(VERBOSE_DEBUG, "-> CMD%02x(%x) tr=%x, cmd queued",
				//	cmd->CommandID, cmd->LingoID, cmd->Transaction);
			}
#ifdef DEBUG
			else _die();	// target cmd buffer unavailable
#endif
		}
		else 
		{
			_verbose(VERBOSE_DEBUG, "-> bad checksum!");
			if (cmd_node != NULL)
			{
				_free_cmd(cmd_node);
			}
			break;
		}
	}
}


iap_request_t *iap_begin_req(iap_cmd_t *cmd, unsigned char *cmd_data, iap_cmd_t *resp, unsigned char *resp_data)
{
	iap_request_t *req = _alloc_request();
	if (req != NULL)
	{
		cmd->Transaction = _transaction++;
		req->Cmd = cmd;
		req->CmdData = cmd_data;
		req->Response = resp;
		req->ResponseData = resp_data;

		exos_mutex_lock(&_busy_requests_lock);
		list_add_tail(&_busy_requests_list, &req->Node);
		exos_mutex_unlock(&_busy_requests_lock);

		if (iap_send_cmd(cmd, cmd_data))
			return req;

		_verbose(VERBOSE_ERROR, "cmd request could not be sent, CMD%02x(%x) tr=%x",
				cmd->CommandID, cmd->LingoID, cmd->Transaction);

		exos_mutex_lock(&_busy_requests_lock);
		list_remove(&req->Node);
		exos_mutex_unlock(&_busy_requests_lock);

		_free_request(req);
	}
	return NULL;
}

iap_cmd_status_t iap_end_req(iap_request_t *req, unsigned timeout)
{
	if (req == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	if (!exos_event_wait(&req->CompletedEvent, timeout))
	{
		exos_mutex_lock(&_busy_requests_lock);
		if (list_find_node(&_busy_requests_list, &req->Node))
			list_remove(&req->Node);
		exos_mutex_unlock(&_busy_requests_lock);
		_free_request(req);
		return IAP_ERROR_OPERATION_TIMED_OUT;
	}

	iap_cmd_status_t status = req->Response->ErrorCode;
	_free_request(req);
	return status;
}

iap_cmd_status_t iap_do_req(iap_cmd_t *cmd, unsigned char *cmd_data, iap_cmd_t *resp, unsigned char *resp_data)
{
	iap_request_t *req = iap_begin_req(cmd, cmd_data, resp, resp_data);
	return (req != NULL) ? iap_end_req(req, 5000) : IAP_ERROR_COMMAND_FAILED;
}

iap_cmd_status_t iap_do_req2(iap_cmd_id_t cmd_id, iap_cmd_t *resp, unsigned char *resp_data)
{
	iap_cmd_t cmd = (iap_cmd_t) { .CommandID = cmd_id };
	return iap_do_req(&cmd, NULL, resp, resp_data);
}

iap_cmd_status_t iap_do_req3(iap_cmd_id_t cmd_id, unsigned char *cmd_data, int cmd_length, iap_cmd_t *resp, unsigned char *resp_data)
{
	iap_cmd_t cmd = (iap_cmd_t) { .CommandID = cmd_id, .Length = cmd_length };
	return iap_do_req(&cmd, cmd_data, resp, resp_data);
}

bool iap_get_incoming_cmd(iap_cmd_t *cmd, unsigned char *cmd_data, int timeout)
{
	iap_cmd_node_t *cmd_node = (iap_cmd_node_t *)exos_fifo_wait(&_incoming_cmds_fifo, timeout);
	if (cmd_node)
	{
		iap_cmd_t *input = &cmd_node->Cmd;
		cmd->LingoID = input->LingoID;
		cmd->ErrorCode = input->ErrorCode;
		cmd->Transaction = input->Transaction;
		cmd->CommandID = input->CommandID;

		int length = input->Length < cmd->Length ? input->Length : cmd->Length;
		for(int i = 0; i < length; i++) cmd_data[i] = cmd_node->Buffer[i];
		cmd->Length = length;

		_free_cmd(cmd_node);
		return true;
	}
	return false;
}

static bool _identify()
{
	iap_cmd_status_t status;
	unsigned char buffer[256];
	iap_cmd_t resp = (iap_cmd_t) { .Length = sizeof(buffer) };
	status = iap_do_req2(IAP_CMD_START_IDPS, &resp, buffer);
    if (status == IAP_OK)
	{
		resp = (iap_cmd_t) { .Length = sizeof(buffer) };
		status = iap_do_req2(IAP_CMD_REQUEST_TRANSPORT_MAX_PAYLOAD_SIZE, &resp, buffer);
		unsigned max_payload = (status == IAP_OK && resp.CommandID == IAP_CMD_RETURN_TRANSPORT_MAX_PAYLOAD_SIZE) ? 
			buffer[0] << 8 | buffer[1] : 
			(status == IAP_ERROR_BAD_PARAMETER ? IAP_MAX_PACKET_BUFFER : 0);
		if (max_payload != 0)
		{
			_verbose(VERBOSE_DEBUG, "max_payload=0x%x", max_payload);
			unsigned char lingo = 0;
			resp = (iap_cmd_t) { .Length = sizeof(buffer) };
			status = iap_do_req3(IAP_CMD_GET_IPOD_OPTIONS_FOR_LINGO, &lingo, 1, &resp, buffer);
			// TODO: use received info when other lingos are supported

			int fid_data = iap_fid_fill(buffer, sizeof(buffer));
			resp = (iap_cmd_t) { .Length = sizeof(buffer) - fid_data };
			status = iap_do_req3(IAP_CMD_SET_FID_TOKEN_VALUES, buffer, fid_data, &resp, buffer + fid_data);
			if (status == IAP_OK)
			{
				unsigned char end_param = 0; // immediately request authentication
				resp = (iap_cmd_t) { .Length = sizeof(buffer) };
				status = iap_do_req3(IAP_CMD_END_IDPS, &end_param, 1, &resp, buffer);
				if (status == IAP_OK)
					return true;
				else _verbose(VERBOSE_ERROR, "IAP_CMD_END_IDPS failed!");
			}
			else _verbose(VERBOSE_ERROR, "IAP_CMD_SET_FID_TOKEN_VALUES failed!");
		}
		else
		{
			_verbose(VERBOSE_ERROR, "IAP_CMD_REQUEST_TRANSPORT_MAX_PAYLOAD_FAILED; max_payload=0x%x, status=0x%02x!", 
				max_payload, status);
		}
	}
	else
	{
		_verbose(VERBOSE_ERROR, "IAP_CMD_START_IDPS failed; status=0x%02x!", status);
	}
	return false;
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
	iap_cmd_t cmd, resp;
	unsigned char cmd_buffer[512];
	unsigned char resp_buffer[256];
	unsigned short total_length;
	unsigned char parts_done;
	int offset;
	int done;
	while(!_service_exit)
	{
		cmd = (iap_cmd_t) { .Length = sizeof(cmd_buffer) };
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
									_verbose(VERBOSE_DEBUG, "AuthInfo part %d", parts_done);
									resp = (iap_cmd_t) { .CommandID = IAP_CMD_RET_ACC_AUTH_INFO, .Length = offset, .Transaction = cmd.Transaction };
									iap_send_cmd(&resp, resp_buffer);
									break;
								case IAP_CMD_ACCESORY_DATA_TRANSFER:
									_verbose(VERBOSE_DEBUG, "AccessoryDataTransfer returned status 0x%02x",
										cmd_buffer[0]);
									break;
								case IAP_CMD_SET_AVAILABLE_CURRENT:
									_verbose(VERBOSE_DEBUG, "SetAvailableCurrent returned status 0x%02x",
										cmd_buffer[0]);
									break;
								default:
									_verbose(VERBOSE_DEBUG, "received unexpected iPodAck for cmd 0x%02x",
										cmd_buffer[1]);
									break;
							}
						}
						else 
						{
							_verbose(VERBOSE_DEBUG, "iPodAck received with status 0x%02x", cmd.ErrorCode);
						}
						break;
					case IAP_CMD_GET_ACC_AUTH_INFO:
						if (apple_cp2_read_acc_cert_length(&total_length))
						{
							_verbose(VERBOSE_ERROR, "CP cert length=%d", total_length);
							parts_done = 0;
							offset = _read_cert(&parts_done, resp_buffer, total_length);
							_verbose(VERBOSE_DEBUG, "AuthInfo part %d (%d bytes)", parts_done, offset);
							resp = (iap_cmd_t) { .CommandID = IAP_CMD_RET_ACC_AUTH_INFO, .Length = offset, .Transaction = cmd.Transaction };
                            iap_send_cmd(&resp, resp_buffer);
						}
						else _verbose(VERBOSE_ERROR, "CP didn't return a valid cert length!");
						break;
					case IAP_CMD_ACK_ACC_AUTH_INFO:
						if (cmd_buffer[0] == 0)	// 0x00 = auth info supported
						{
							// hmmmm, nothing to do but wait next command							 
						}
						else _verbose(VERBOSE_ERROR, "Device rejected auth info ($%02x)!", cmd_buffer[0]);
						break;
					case IAP_CMD_GET_ACC_AUTH_SIGNATURE:
						if (cmd.Length == 21)
						{
							// NOTE: what is the last byte for?
							_verbose(VERBOSE_DEBUG, "Authentication started!");
							offset = apple_cp2_get_auth_signature(cmd_buffer, cmd.Length - 1, resp_buffer);
							resp = (iap_cmd_t) { .CommandID = IAP_CMD_RET_ACC_AUTH_SIGNATURE, .Length = offset, .Transaction = cmd.Transaction };
							iap_send_cmd(&resp, resp_buffer);
						}
						else _verbose(VERBOSE_ERROR, "Auth challenge length not supported (%d)!", cmd.Length);
						break;
					case IAP_CMD_ACK_ACC_AUTH_STATUS:
						if (cmd.Length == 1 && 
							cmd_buffer[0] == 0) // auth operation passed
						{
							_verbose(VERBOSE_DEBUG, "Authentication completed!");

							// EXPERIMENTAL: try to enable ipad charging using iap 
							resp_buffer[offset++] = 2100 >> 8;
							resp_buffer[offset++] = 2100 & 0xFF;
							resp = (iap_cmd_t) { .CommandID = IAP_CMD_SET_AVAILABLE_CURRENT, .Length = offset, .Transaction = cmd.Transaction };
							iap_send_cmd(&resp, resp_buffer);

							// TODO: notify app level that lingo ids are ready to be used
						}
#ifdef DEBUG
						else 
						{
							_verbose(VERBOSE_ERROR, "Authetication failed ($%02x)!!!", cmd_buffer[0]);
							//_die();	// auth failed, FIXME <<<<<<<<<<<<<<<
						}
#endif
						break;
					case IAP_CMD_OPEN_DATA_SESSION_FOR_PROTOCOL:
						_verbose(VERBOSE_DEBUG, "OpenDataSession (0x%02x) tr=%x",
							cmd.CommandID, cmd.Transaction);

						// FIXME: ensure we are not ignoring protocol field
						
						done = iap_open_session((cmd_buffer[0] << 8) | cmd_buffer[1], cmd_buffer[2]);
						resp_buffer[offset++] = done ? IAP_OK : IAP_ERROR_BAD_PARAMETER;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (iap_cmd_t) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = cmd.Transaction };
                        iap_send_cmd(&resp, resp_buffer);
						break;
					case IAP_CMD_IPOD_DATA_TRANSFER:
						_verbose(VERBOSE_DEBUG, "iPodDataTransfer (0x%02x) tr=%x, length=%d",
							cmd.CommandID, cmd.Transaction, cmd.Length);

						if (cmd.Length > 2)
						{
							done = iap_comm_write((cmd_buffer[0] << 8) | cmd_buffer[1], &cmd_buffer[2], cmd.Length - 2);
						}
						else done = 0;

						resp_buffer[offset++] = done ? IAP_OK : IAP_ERROR_BAD_PARAMETER;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (iap_cmd_t) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = cmd.Transaction };
						iap_send_cmd(&resp, resp_buffer);
						break;
					case IAP_CMD_CLOSE_DATA_SESSION:
						_verbose(VERBOSE_DEBUG, "CloseDataSession (0x%02x) tr=%x",
							cmd.CommandID, cmd.Transaction);

						iap_close_session((cmd_buffer[0] << 8) | cmd_buffer[1]);
						resp_buffer[offset++] = IAP_OK;
						resp_buffer[offset++] = cmd.CommandID;
						resp = (iap_cmd_t) { .CommandID = IAP_CMD_ACCESORY_ACK, .Length = offset, .Transaction = cmd.Transaction };
                        iap_send_cmd(&resp, resp_buffer);
						break;
					default:
						_verbose(VERBOSE_DEBUG, "unexpected command(0x%02x) tr=%x",
							cmd.CommandID, cmd.Transaction);
						break;
				}
			}
		}
	}
	return 0;
}

static void *_service(void *arg)
{
	exos_thread_sleep(1000);

	_verbose(VERBOSE_COMMENT, "Service started...");

	if (_identify())
	{
		_verbose(VERBOSE_DEBUG, "Identify procedure succeded!");

		_slave_io();
		iap_close_all();
	}
	else
	{
		_verbose(VERBOSE_ERROR, "Identify procedure failed!");
	}
	_verbose(VERBOSE_COMMENT, "Service exiting...");
	_service_exit = 0;
}



