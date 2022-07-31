#ifndef EXOS_CANOPEN_H
#define EXOS_CANOPEN_H

#include <support/misc/can_receiver.h>
#include <kernel/thread.h>

#define __packed __attribute__((__packed__))

typedef struct __packed
{
	unsigned short Index;
	unsigned char SubIndex;
} CANOPEN_MUX;

typedef enum
{
	CANOPEN_NODE_BOOTUP = 0,
	CANOPEN_NODE_PRE_OPERATIONAL = 0x7F,
	CANOPEN_NODE_OPERATIONAL = 5,
	CANOPEN_NODE_STOPPED = 4,
} CANOPEN_NODE_STATE;

typedef struct __packed
{
	unsigned char Cmd;
	unsigned char NodeId;
} CANOPEN_NMT_MSG;

typedef struct __packed
{
	union
	{
		struct
		{
			unsigned s:1;
			unsigned e:1;
			unsigned n:2;
			unsigned :1;
			unsigned ccs:3;
			unsigned index:16;
			unsigned subindex:8;
		};
		unsigned long Value;
	} Header;
	unsigned long Data;
} CANOPEN_SDO_MSG;

typedef enum
{
	CANOPEN_NMT_START_NODE = 1,
	CANOPEN_NMT_STOP_NODE = 2,
	CANOPEN_NMT_PRE_OPERATIONAL = 0x80,
	CANOPEN_NMT_RESET_NODE = 0x81,
	CANOPEN_NMT_RESET_COMM = 0x82,
} CANOPEN_NMT_CMD;

typedef struct
{
	unsigned COB_ID:29;
	unsigned Extended:1;
	unsigned AllowRTR:1;
	unsigned Enabled:1;
} CANOPEN_PDO_COMM_PARAM_1;


#define CANOPEN_ID(a, b, c, d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))

typedef struct
{
	unsigned long DeviceType;
	unsigned long VendorID;
	unsigned long ProductCode;
	unsigned long Revision;
	unsigned long SerialNumber;
	const char *DeviceName;
	const char *HwVersion;
	const char *SwVersion;
	unsigned char Node;
} CANOPEN_NODE_IDENTITY;

typedef struct
{
	EXOS_NODE;
	const CANOPEN_NODE_IDENTITY *Identity;
	CANOPEN_NODE_STATE State;
	struct
	{
		unsigned NodeGuardToggle:1;
	} Flags;
} CANOPEN_INSTANCE;

// prototypes
int canopen_initialize(int can_module, int bitrate);
int canopen_slave_create(CANOPEN_INSTANCE *ci, const CANOPEN_NODE_IDENTITY *identity);
int canopen_slave_add(CANOPEN_INSTANCE *ci);

int canopen_master_sync();
int canopen_nmt_send_cmd(int cmd, int target_node);

int canopen_sdo_read(int target_node, CANOPEN_MUX mux, void *data);
int canopen_sdo_read_expedited(int target_node, CANOPEN_MUX mux, unsigned long *data);

#endif // EXOS_CANOPEN_H
