#ifndef APPLE_IAP_CORE_H
#define APPLE_IAP_CORE_H

#include <kernel/event.h>

typedef enum
{
	IAP_LINGO_GENERAL = 0,
} iap_lingo_t;

typedef enum
{
	// General Lingo Commands
	IAP_CMD_REQUEST_IDENTIFY = 0,
	IAP_CMD_IDENTIFY_DEPRECATED,
	IAP_CMD_IPODACK,

	IAP_CMD_REQUEST_LINGO_PROTOCOL_VERSION = 0x0F,
	IAP_CMD_RETURN_LINGO_PROTOCOL_VERSION,
	IAP_CMD_REQUEST_TRANSPORT_MAX_PAYLOAD_SIZE,
	IAP_CMD_RETURN_TRANSPORT_MAX_PAYLOAD_SIZE,

	IAP_CMD_GET_ACC_AUTH_INFO = 0x14,
	IAP_CMD_RET_ACC_AUTH_INFO,
	IAP_CMD_ACK_ACC_AUTH_INFO,
	IAP_CMD_GET_ACC_AUTH_SIGNATURE,
	IAP_CMD_RET_ACC_AUTH_SIGNATURE,
	IAP_CMD_ACK_ACC_AUTH_STATUS,

	IAP_CMD_START_IDPS = 0x38,
	IAP_CMD_SET_FID_TOKEN_VALUES,
	IAP_CMD_ACK_FID_TOKEN_VALUES,
	IAP_CMD_END_IDPS,
	IAP_CMD_IDPS_STATUS,
	
	IAP_CMD_OPEN_DATA_SESSION_FOR_PROTOCOL = 0x3F,
	IAP_CMD_CLOSE_DATA_SESSION,
	IAP_CMD_ACCESORY_ACK,
	IAP_CMD_ACCESORY_DATA_TRANSFER,
	IAP_CMD_IPOD_DATA_TRANSFER,

	IAP_CMD_IPOD_NOTIFICATION = 0x4A,
	IAP_CMD_GET_IPOD_OPTIONS_FOR_LINGO,
	IAP_CMD_RET_IPOD_OPTIONS_FOR_LINGO,
	IAP_CMD_GET_EVENT_NOTIFICATION,
	IAP_CMD_RET_EVENT_NOTIFICATION,
	IAP_CMD_GET_SUPPORTED_EVENT_NOTIFICATION,
	IAP_CMD_CANCEL_COMMAND,
	IAP_CMD_RET_SUPPORTED_EVENT_NOTIFICATION,

	IAP_CMD_SET_AVAILABLE_CURRENT = 0x54,
} iap_cmd_id_t;

typedef enum
{
	IAP_OK = 0,
	IAP_ERROR_UNKNOWN_CATEGORY_OR_SESSION_ID,
	IAP_ERROR_COMMAND_FAILED,
	IAP_ERROR_DEVICE_OUT_OF_RESOURCES,
	IAP_ERROR_BAD_PARAMETER,
	IAP_ERROR_UNKNOWN_ID,
	IAP_ERROR_COMMAND_PENDING,
	IAP_ERROR_NOT_AUTHENTICATED,
	IAP_ERROR_BAD_AUTH_VERSION,
	IAP_ERROR_POWER_MODE_REQ_FAILED,
	IAP_ERROR_CERTIFICATE_INVALID,
	IAP_ERROR_CERTIFICATE_PERMISSIONS_INVALID,
	IAP_ERROR_FILE_IN_USE,
	IAP_ERROR_INVALID_FILE_HANDLE,
	IAP_ERROR_DIRECTORY_NOT_EMPTY,
	IAP_ERROR_OPERATION_TIMED_OUT,
	IAP_ERROR_COMMAND_NOT_AVAILABLE,
	// FIXME: incomplete
} iap_cmd_status_t;

#define IAP_MAX_PACKET_BUFFER 506
typedef unsigned char iap_payload_buffer_t[IAP_MAX_PACKET_BUFFER]; 

typedef struct
{
	unsigned char LingoID;
	unsigned char ErrorCode;
	unsigned short Transaction;
	unsigned short CommandID;
	unsigned short Length;
} iap_cmd_t;

typedef struct
{
	node_t Node;
	iap_cmd_t Cmd;
	iap_payload_buffer_t Buffer;
} iap_cmd_node_t;

typedef struct
{
	node_t Node;
	event_t CompletedEvent;
	iap_cmd_t *Cmd;
	unsigned char *CmdData;
	iap_cmd_t *Response;
	unsigned char *ResponseData;
} iap_request_t;

void iap_core_initialize();
void iap_core_start();
void iap_core_stop();
void iap_core_parse(unsigned char *buffer, int length);

iap_request_t *iap_begin_req(iap_cmd_t *cmd, unsigned char *cmd_data, iap_cmd_t *resp, unsigned char *resp_data);
iap_cmd_status_t iap_end_req(iap_request_t *req, unsigned timeout);
iap_cmd_status_t iap_do_req(iap_cmd_t *cmd, unsigned char *cmd_data, iap_cmd_t *resp, unsigned char *resp_data);
iap_cmd_status_t iap_do_req2(iap_cmd_id_t cmd_id, iap_cmd_t *resp, unsigned char *resp_data);
iap_cmd_status_t iap_do_req3(iap_cmd_id_t cmd_id, unsigned char *cmd_data, int cmd_length, iap_cmd_t *resp, unsigned char *resp_data);

// external
int iap_send_cmd(iap_cmd_t *cmd, unsigned char *data);

#endif // APPLE_IAP_CORE_H


