#include "comm.h"
#include "board.h"
#include <kernel/panic.h>

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const EXOS_IO_DRIVER _comm_driver = {
	.Read = _read, .Write = _write };

void comm_initialize()
{
	// add board devices to device tree
	EXOS_TREE_DEVICE *device;
	for(int index = 0; 
		NULL != (device = comm_board_get_device(index));
		index++)
	{
		device->Type = EXOS_TREE_NODE_DEVICE;
		exos_tree_add_child_path((EXOS_TREE_NODE *)device, "dev");
	}
}

void comm_add_device(EXOS_TREE_DEVICE *device, const char *parent_path)
{
    device->Type = EXOS_TREE_NODE_DEVICE;
    exos_tree_add_child_path((EXOS_TREE_NODE *)device, parent_path);
}


void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port, EXOS_IO_FLAGS flags)
{
	if (io == NULL || device == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_COMM, &_comm_driver, flags);
	io->Device = device;
	io->Port = port;
}

int comm_io_open(COMM_IO_ENTRY *io)
{
#ifdef DEBUG
	if (io == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	
	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	int done = driver->Open(io);
	return done;
}

void comm_io_close(COMM_IO_ENTRY *io)
{
#ifdef DEBUG
	if (io == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	driver->Close(io);
}

int comm_io_get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
#ifdef DEBUG
	if (io == NULL || value == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	int done = driver->GetAttr(io, attr, value);
	return done;
}

int comm_io_set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
#ifdef DEBUG
	if (io == NULL || value == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	int done = driver->SetAttr(io, attr, value);
	return done;
}


static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
	COMM_DEVICE *device = ((COMM_IO_ENTRY *)io)->Device;

	const COMM_DRIVER *driver = device->Driver;
	int done = driver->Read((COMM_IO_ENTRY *)io, buffer, length);
	return done;
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
	COMM_DEVICE *device = ((COMM_IO_ENTRY *)io)->Device;

	const COMM_DRIVER *driver = device->Driver;
	int done = driver->Write((COMM_IO_ENTRY *)io, buffer, length);
	return done;
}


// defined in board_support
__weak EXOS_TREE_DEVICE *comm_board_get_device(int index)
{
	return NULL;
}
