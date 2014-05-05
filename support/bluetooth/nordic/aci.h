#ifndef NORDIC_ACI_H
#define NORDIC_ACI_H

#include <kernel/port.h>
#include <kernel/event.h>

typedef enum
{
	ACI_REQUEST_QUEUED = 0,
	ACI_REQUEST_PENDING,
	ACI_REQUEST_DONE,
	ACI_REQUEST_CANCELLED,
} ACI_REQUEST_STATE;

typedef struct
{
	EXOS_MESSAGE;
	ACI_REQUEST_STATE State;
	EXOS_EVENT Done;
	union
	{
		unsigned char Command;
		unsigned char Status;
	};
	unsigned char Length;
	unsigned char Data[32];
} ACI_REQUEST;

typedef enum
{
	ACI_STATE_RESET = 0,
	ACI_STATE_SETUP,
	ACI_STATE_STANDBY,
	ACI_STATE_ACTIVE,	// (advertisement or connected)
	ACI_STATE_SLEEP,
	ACI_STATE_TEST,
} ACI_DEVICE_STATE;

typedef enum
{
	ACI_EVENT_DEVICE_STARTED = 0x81,
	ACI_EVENT_ECHO = 0x82,
	ACI_EVENT_HW_ERROR = 0x83,
	ACI_EVENT_COMMAND_RESPONSE = 0x84,
	ACI_EVENT_CONNECTED = 0x85,
	ACI_EVENT_DISCONNECTED = 0x86,
	ACI_EVENT_BOND_STATUS = 0x87,
	ACI_EVENT_PIPE_STATUS = 0x88,
	ACI_EVENT_TIMING = 0x89,
	ACI_EVENT_DATA_CREDIT = 0x8A,
	ACI_EVENT_DATA_ACK = 0x8B,
	ACI_EVENT_DATA_RECEIVED = 0x8C,
	ACI_EVENT_PIPE_ERROR = 0x8D,
	ACI_EVENT_DISPLAY_KEY = 0x8E,
	ACI_EVENT_KEY_REQUEST = 0x8F,
} ACI_EVENT;

typedef enum
{
	ACI_BOND_STATUS_SUCCESS = 0,
	ACI_BOND_STATUS_FAILED = 0x01,
	ACI_BOND_STATUS_FAILED_TIMEOUT = 0x02,
	ACI_BOND_STATUS_FAILED_PASSKEY_ENTRY_FAILED = 0x81,
	ACI_BOND_STATUS_FAILED_OOB_UNAVAILABLE = 0x82,
	ACI_BOND_STATUS_FAILED_AUTHENTICATION_REQ = 0x83,
	ACI_BOND_STATUS_FAILED_CONFIRM_VALUE = 0x84,
	ACI_BOND_STATUS_FAILED_PAIRING_UNSUPPORTED = 0x85,
	ACI_BOND_STATUS_FAILED_ENCRYPTION_KEY_SIZE = 0x86,
	ACI_BOND_STATUS_FAILED_SMP_CMD_UNSUPPORTED = 0x87,
	ACI_BOND_STATUS_FAILED_UNSPECIFIED_REASON = 0x88,
	ACI_BOND_STATUS_FAILED_REPEATED_ATTEMPTS = 0x89,
	ACI_BOND_STATUS_FAILED_INVALID_PARAMETERS = 0x8A,
} ACI_BOND_STATUS;

typedef enum
{
	ACI_STATUS_SUCCESS = 0x00,
	ACI_STATUS_TRANSACTION_CONTINUE = 0x01,
	ACI_STATUS_TRANSACTION_COMPLETE = 0x02,
	ACI_STATUS_EXTENDED = 0x03,

	ACI_STATUS_ERROR_UNKNOWN = 0x80,
	ACI_STATUS_ERROR_INTERNAL = 0x81,
	ACI_STATUS_ERROR_CMD_UNKNOWN = 0x82,
	ACI_STATUS_ERROR_DEVICE_STATE_INVALID = 0x83,
	ACI_STATUS_ERROR_INVALID_LENGTH = 0x84,
	ACI_STATUS_ERROR_INVALID_PARAMETER = 0x85,
	ACI_STATUS_ERROR_BUSY = 0x86,
	ACI_STATUS_ERROR_INVALID_DATA = 0x87,
	ACI_STATUS_ERROR_CRC_MISMATCH = 0x88,
	ACI_STATUS_ERROR_UNSUPPORTED_SETUP_FORMAT = 0x89,
	ACI_STATUS_ERROR_INVALID_SEQ_NO = 0x8A,
	ACI_STATUS_ERROR_SETUP_LOCKED = 0x8B,
	ACI_STATUS_ERROR_LOCK_FAILED = 0x8C,
	ACI_STATUS_ERROR_BOND_REQUIRED = 0x8D,
	ACI_STATUS_ERROR_REJECTED = 0x8E,
	ACI_STATUS_ERROR_DATA_SIZE = 0x8F,
	ACI_STATUS_ERROR_PIPE_INVALID = 0x90,
	ACI_STATUS_ERROR_CREDIT_NOT_AVAILABLE = 0x91,
	ACI_STATUS_ERROR_PEER_ATT_ERROR = 0x92,
	ACI_STATUS_ERROR_ADVT_TIMEOUT = 0x93,
	ACI_STATUS_ERROR_PEER_SMP_ERROR = 0x94,
	ACI_STATUS_ERROR_PIPE_TYPE_INVALID = 0x95,
	ACI_STATUS_ERROR_PIPE_STATE_INVALID = 0x96,
	ACI_STATUS_ERROR_INVALID_KEY_SIZE = 0x97,
	ACI_STATUS_ERROR_INVALID_KEY_DATA = 0x98,
} ACI_STATUS_CODE;

typedef enum
{
	ACI_COMMAND_TEST = 0x01,
	ACI_COMMAND_ECHO = 0x02,
	ACI_COMMAND_DTM_COMMAND = 0x03,
	ACI_COMMAND_SLEEP = 0x04,
	ACI_COMMAND_WAKEUP = 0x05,
	ACI_COMMAND_SETUP = 0x06,
	ACI_COMMAND_READ_DYNAMIC_DATA = 0x07,
	ACI_COMMAND_WRITE_DYNAMIC_DATA = 0x08,
	ACI_COMMAND_GET_DEVICE_VERSION = 0x09,
	ACI_COMMAND_GET_DEVICE_ADDRESS = 0x0A,
	ACI_COMMAND_GET_BATTERY_LEVEL = 0x0B,
	ACI_COMMAND_GET_TEMPERATURE = 0x0C,
	
	ACI_COMMAND_SET_LOCAL_DATA = 0x0D,

	ACI_COMMAND_RADIO_RESET = 0x0E,
	ACI_COMMAND_CONNECT = 0x0F,
	ACI_COMMAND_BOND = 0x10,
	ACI_COMMAND_DISCONNECT = 0x11,
	ACI_COMMAND_SET_TX_POWER = 0x12,
	ACI_COMMAND_CHANGE_TIMING_REQ = 0x13,
	ACI_COMMAND_OPEN_REMOTE_PIPE = 0x14,

	ACI_COMMAND_SEND_DATA = 0x15,
	ACI_COMMAND_SEND_DATA_ACK = 0x16,
	ACI_COMMAND_REQUEST_DATA = 0x17,
	ACI_COMMAND_SEND_DATA_NACK = 0x18,

	ACI_COMMAND_SET_APPL_LATENCY = 0x19,
	ACI_COMMAND_SET_KEY = 0x1A,
	ACI_COMMAND_OPEN_ADV_PIPE = 0x1B,
	ACI_COMMAND_BROADCAST = 0x1C,
	ACI_COMMAND_BOND_SECURITY_REQ = 0x1D,
	ACI_COMMAND_DIRECTED_CONNECT = 0x1E,
	ACI_COMMAND_CLOSE_REMOTE_PIPE = 0x1F,
} ACI_COMMAND;

typedef struct
{
	unsigned char CommandOpCode;
	unsigned char Status;
	unsigned char ResponseData[];
} ACI_COMMAND_RESPONSE_EVENT_DATA;

typedef struct
{
	unsigned short Timeout;
	unsigned short AdvInterval;
} ACI_CONNECT_COMMAND_DATA;

typedef struct
{
	unsigned char AddressType;
	unsigned char Address[6];
	// TODO
} ACI_CONNECTED_EVENT_DATA;

typedef struct
{
	unsigned char PipesOpen[8];
	unsigned char PipesClosed[8];
} ACI_PIPE_STATUS_EVENT_DATA;

typedef struct
{
	unsigned char Reason;
	// TODO
} ACI_DISCONNECTED_EVENT_DATA;

typedef struct
{
	unsigned char Pipe;
	unsigned char ErrorCode;
} ACI_PIPE_ERROR_EVENT_DATA;

typedef struct
{
	unsigned char Pipe;
	unsigned char Data[];
} ACI_DATA_RECEIVED_EVENT_DATA;

void aci_initialize();
int aci_send_setup(ACI_REQUEST *req, int *pcomplete);
int aci_broadcast(unsigned short adv_interval);
int aci_connect(unsigned short adv_interval);
int aci_connect_wait(unsigned short adv_interval, unsigned int timeout);
int aci_is_connected();
int aci_set_local_data(unsigned char pipe, unsigned char *data, int length);
int aci_send_data(unsigned char pipe, unsigned char *data, int length);

#endif // NORDIC_ACI_H


