#include "comm.h"
#include <kernel/panic.h>
#include <support/comm_hal.h>

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const EXOS_IO_DRIVER _comm_driver = {
	.Read = _read, .Write = _write };

void comm_initialize()
{
	// add board devices to device tree
	EXOS_TREE_DEVICE *device;
	for(int index = 0; 
		NULL != (device = hal_comm_get_device(index));
		index++)
	{
		device->Type = EXOS_TREE_NODE_DEVICE;
		exos_tree_add_device(device);
	}
}

void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port, EXOS_IO_FLAGS flags)
{
	if (io == NULL || device == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_COMM, &_comm_driver, flags);
	io->Device = device;
	io->Port = port;
	io->Baudrate = 0;
}

int comm_io_open(COMM_IO_ENTRY *io, int baudrate)
{
	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	io->Baudrate = baudrate;

	int done = driver->Open(io);
	return done;
}

void comm_io_close(COMM_IO_ENTRY *io)
{
	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	driver->Close(io);
}

int comm_io_set_baudrate(COMM_IO_ENTRY *io, int baudrate)
{
	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	io->Baudrate = baudrate;

	int done = driver->SetAttrs(io);
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

__weak EXOS_TREE_DEVICE *hal_comm_get_device(int index)
{
	return NULL;
}
