#ifndef COMM_COMM_H
#define COMM_COMM_H

#include <stdbool.h>
#include <kernel/io.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <kernel/tree.h>

#ifndef COMM_DEVICE_THREAD_STACK
#define COMM_DEVICE_THREAD_STACK 512
#endif

typedef io_driver_t COMM_DRIVER __deprecated;
typedef io_tree_device_t EXOS_TREE_DEVICE __deprecated;
typedef io_entry_t COMM_IO_ENTRY __deprecated;

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


void comm_initialize();
//void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port/*, io_flags_t flags*/);
bool comm_io_create_from_path(COMM_IO_ENTRY *io, const char *path, io_flags_t flags);
int comm_io_open(COMM_IO_ENTRY *io);
void comm_io_close(COMM_IO_ENTRY *io);
bool comm_io_get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
bool comm_io_set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);


#endif // COMM_COMM_H
