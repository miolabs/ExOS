#ifndef APPLE_IAP_CORE_H
#define APPLE_IAP_CORE_H

#include <kernel/event.h>

typedef enum
{
	IAP_LINGO_GENERAL = 0,
} IAP_LINGO_ID;

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
	IAP_CMD_IPOD_NOTIFICATION,
} IAP_CMD_ID;

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
} IAP_CMD_STATUS;

#define IAP_MAX_PACKET_BUFFER 506
typedef unsigned char IAP_PAYLOAD_BUFFER[IAP_MAX_PACKET_BUFFER]; 

typedef struct
{
	unsigned char LingoID;
	unsigned char ErrorCode;
	unsigned short Transaction;
	unsigned short CommandID;
	unsigned short Length;
} IAP_CMD;

typedef struct
{
	EXOS_NODE Node;
	IAP_CMD Cmd;
	IAP_PAYLOAD_BUFFER Buffer;
} IAP_CMD_NODE;

typedef struct
{
	EXOS_NODE Node;
	EXOS_EVENT CompletedEvent;
	IAP_CMD *Cmd;
	unsigned char *CmdData;
	IAP_CMD *Response;
	unsigned char *ResponseData;
} IAP_REQUEST;

void iap_core_initialize();
void iap_core_start();
void iap_core_stop();
void iap_core_parse(unsigned char *buffer, int length);

IAP_REQUEST *iap_begin_req(IAP_CMD *cmd, unsigned char *cmd_data, IAP_CMD *resp, unsigned char *resp_data);
IAP_CMD_STATUS iap_end_req(IAP_REQUEST *req, int timeout);
IAP_CMD_STATUS iap_do_req(IAP_CMD *cmd, unsigned char *cmd_data, IAP_CMD *resp, unsigned char *resp_data);
IAP_CMD_STATUS iap_do_req2(IAP_CMD_ID cmd_id, IAP_CMD *resp, unsigned char *resp_data);
IAP_CMD_STATUS iap_do_req3(IAP_CMD_ID cmd_id, unsigned char *cmd_data, int cmd_length, IAP_CMD *resp, unsigned char *resp_data);

// external
int iap_send_cmd(IAP_CMD *cmd, unsigned char *data);

#endif // APPLE_IAP_CORE_H


