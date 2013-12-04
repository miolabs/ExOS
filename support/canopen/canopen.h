#ifndef EXOS_CANOPEN_H
#define EXOS_CANOPEN_H

#include <support/misc/can_receiver.h>
#include <kernel/thread.h>

typedef struct
{
	unsigned short Index;
	unsigned char SubIndex;
} CANOPEN_MUX;

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
	unsigned char NodeID;
} CANOPEN_NODE_IDENTITY;

typedef struct
{
	CAN_HANDLER Handler;
	EXOS_THREAD Thread;
	const CANOPEN_NODE_IDENTITY *Identity;
} CANOPEN_INSTANCE;

// prototypes
int canopen_create(CANOPEN_INSTANCE *ci, const CANOPEN_NODE_IDENTITY *identity, int can_module, int bitrate);
int canopen_run(CANOPEN_INSTANCE *ci, int pri, void *stack, int stack_size);
int canopen_sdo_download(CANOPEN_INSTANCE *ci, int cob_id, CANOPEN_MUX mux, void *data);
int canopen_sdo_download_expedited(CANOPEN_INSTANCE *ci, int cob_id, CANOPEN_MUX mux, unsigned long data);

#endif // EXOS_CANOPEN_H
