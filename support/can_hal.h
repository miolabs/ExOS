#ifndef HAL_CAN_HAL_H
#define HAL_CAN_HAL_H

typedef union
{
	unsigned char u8[8];
	short u16[4];
	long u32[2];
    float flo[2];
} CAN_BUFFER; 

typedef struct
{
	unsigned Id : 29;
	unsigned Bus : 2;
} CAN_EP;

typedef enum
{
	CANF_NONE = 0,
	CANF_RXINT = 1<<0,
	CANF_RTR = 1<<1,
	CANF_EXTID = 1<<2,
} CAN_MSG_FLAGS;

typedef struct
{
	CAN_EP EP;
	unsigned Length;
	CAN_MSG_FLAGS Flags;
	CAN_BUFFER Data;
} CAN_MSG;

typedef enum
{
	CAN_INITF_NONE = 0,
	CAN_INITF_DISABLE_RETRANSMISSION = 1<<0,
	CAN_INITF_LISTEN_ONLY = 1<<1,
} CAN_INIT_FLAGS;

typedef void (* HAL_CAN_HANDLER)(int module);

typedef enum
{
	FULLCAN_SETUP_END = 0,
	FULLCAN_SETUP_RX,
	FULLCAN_SETUP_TX,
} FULLCAN_SETUP_CODE;

typedef FULLCAN_SETUP_CODE (* HAL_FULLCAN_SETUP_CALLBACK)(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

// prototypes
int hal_can_initialize(int module, int bitrate, CAN_INIT_FLAGS initf);
int hal_can_send(CAN_EP ep, CAN_BUFFER *data, unsigned char length, CAN_MSG_FLAGS flags);
void hal_can_cancel_tx();

int hal_fullcan_setup(HAL_FULLCAN_SETUP_CALLBACK callback, void *state);
int hal_fullcan_read_msg(int index, CAN_MSG *msg);
int hal_fullcan_write_msg(int index, CAN_MSG *msg);
int hal_fullcan_write_data(int index, CAN_BUFFER *data, int length);

// callbacks
void hal_can_received_handler(int index, CAN_MSG *msg) __attribute__((__weak__));
void hal_can_sent_handler(int module) __attribute__((__weak__));

#endif // HAL_CAN_HAL_H
