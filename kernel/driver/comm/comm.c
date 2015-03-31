#include "comm.h"
#include <kernel/panic.h>

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);
static int _sync(EXOS_IO_ENTRY *io);

static const EXOS_IO_DRIVER _comm_driver = {
	.Read = _read, .Write = _write, .Sync = _sync };

void comm_initialize()
{
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
	exos_event_create(&io->SyncEvent);
	io->Device = device;
	io->Port = port;
}

int comm_io_open(COMM_IO_ENTRY *io)
{
#ifdef DEBUG
	if (io == NULL) 
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
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
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
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
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
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
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif

	COMM_DEVICE *device = io->Device;
	const COMM_DRIVER *driver = device->Driver;
	int done = driver->SetAttr(io, attr, value);
	return done;
}


static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	COMM_IO_ENTRY *comm = (COMM_IO_ENTRY *)io;
	COMM_DEVICE *device = comm->Device;

	const COMM_DRIVER *driver = device->Driver;
	int done = driver->Read(comm, buffer, length);
	return done;
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	COMM_IO_ENTRY *comm = (COMM_IO_ENTRY *)io;
	COMM_DEVICE *device = comm->Device;

	const COMM_DRIVER *driver = device->Driver;
	return driver->Write(comm, buffer, length);
}

static int _sync(EXOS_IO_ENTRY *io)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_COMM)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	COMM_IO_ENTRY *comm = (COMM_IO_ENTRY *)io;
	int res = exos_event_wait(&comm->SyncEvent, io->Timeout);	// NOTE: timeout 0 will never timeout
	return res;
}




