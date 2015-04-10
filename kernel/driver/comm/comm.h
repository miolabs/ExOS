#ifndef COMM_COMM_H
#define COMM_COMM_H

#include <kernel/io.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <kernel/tree.h>

#ifndef COMM_DEVICE_THREAD_STACK
#define COMM_DEVICE_THREAD_STACK 512
#endif

typedef struct _COMM_DRIVER COMM_DRIVER;

typedef struct
{
	const COMM_DRIVER *Driver;
	unsigned PortCount;
} COMM_DEVICE;

typedef struct
{
	EXOS_TREE_NODE;
	COMM_DEVICE *Device;
	unsigned long Unit;
} EXOS_TREE_DEVICE;

typedef struct 
{
	EXOS_IO_ENTRY;
	EXOS_EVENT SyncEvent;
	COMM_DEVICE *Device;
	unsigned Port;
} COMM_IO_ENTRY;

typedef enum
{
	COMM_ATTR_BAUDRATE = 0,
	COMM_ATTR_HANDSHAKE,
	COMM_ATTR_RS485_CONFIG,
	COMM_ATTR_ADDRESS,
	COMM_ATTR_PRINTER_STATUS,
	COMM_ATTR_SESSION_ID,
} COMM_ATTR_ID;

typedef struct
{
	struct 
	{
		unsigned DirectionRTS:1;
		unsigned DirectionDTR:1;
		unsigned DirectionInverted:1;
		unsigned AutoAddressDetect:1;
	} Mode;
	unsigned short Address;
	unsigned short DirectionDisableDelay;
} COMM_RS485_CONFIG;

struct _COMM_DRIVER
{
	int (*Open)(COMM_IO_ENTRY *io);
	void (*Close)(COMM_IO_ENTRY *io);
	int (*GetAttr)(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
	int (*SetAttr)(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
	int (*Read)(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
	int (*Write)(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length);
};


void comm_initialize();
void comm_add_device(EXOS_TREE_DEVICE *device, const char *parent_path);
void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port, EXOS_IO_FLAGS flags);
int comm_io_create_from_path(COMM_IO_ENTRY *io, const char *path, EXOS_IO_FLAGS flags);
int comm_io_open(COMM_IO_ENTRY *io);
void comm_io_close(COMM_IO_ENTRY *io);
int comm_io_get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
int comm_io_set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);


#endif // COMM_COMM_H
