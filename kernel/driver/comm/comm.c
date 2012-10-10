#include "comm.h"
#include "comm_service.h"
#include <support/comm_hal.h>

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);

static const EXOS_IO_DRIVER _comm_driver = {
	.Read = _read };

void comm_initialize()
{
	int index = 0;
	COMM_DEVICE *device;
	while(NULL != (device = hal_comm_get_device(index)))
	{
		// TODO
		index++;
	}
}

void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port, EXOS_IO_FLAGS flags)
{
	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_COMM, &_comm_driver, flags);
	io->Device = device != NULL ? device : hal_comm_get_device(0);
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

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
	COMM_DEVICE *device = ((COMM_IO_ENTRY *)io)->Device;
	const COMM_DRIVER *driver = device->Driver;
	int done = driver->Read((COMM_IO_ENTRY *)io, buffer, length);
	return done;
}
