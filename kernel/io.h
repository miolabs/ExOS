#ifndef EXOS_IO_H
#define EXOS_IO_H

#include <kernel/event.h>

typedef struct _EXOS_IO_DRIVER EXOS_IO_DRIVER;

typedef enum
{
	EXOS_IO_FILE = 0,
	EXOS_IO_SOCKET,
} EXOS_IO_TYPE;

typedef enum
{
	EXOS_IOF_NONE = 0,
	EXOS_IOF_WAIT = (1<<0),
} EXOS_IO_FLAGS;

typedef struct 
{
	EXOS_NODE Node;
	EXOS_IO_TYPE Type;
	const EXOS_IO_DRIVER *Driver;
	EXOS_IO_FLAGS Flags;
	EXOS_EVENT InputEvent;
} EXOS_IO_ENTRY;

struct _EXOS_IO_DRIVER
{
	int (*Read)(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
};


void exos_io_create(EXOS_IO_ENTRY *io, EXOS_IO_TYPE type, const EXOS_IO_DRIVER *driver, EXOS_IO_FLAGS flags);

#endif // EXOS_IO_H
