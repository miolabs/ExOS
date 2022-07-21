#include "io.h"
#include <kernel/panic.h>
#include <kernel/tree.h>
#include <kernel/event.h>

#ifndef EXOS_NO_NET
#include <net/net.h>
#endif

void __io_initialize()
{
	__tree_initialize();
//	comm_initialize();
#ifndef EXOS_NO_NET
	net_initialize();
#endif
}


void exos_io_add_device(io_tree_device_t *device, const char *name, const io_driver_t *driver, void *driver_context)
{
	ASSERT(device != NULL && driver != NULL, KERNEL_ERROR_NULL_POINTER);
	device->TreeNode.Type = EXOS_TREE_NODE_DEVICE;
	device->TreeNode.Name = name;
	device->Driver = driver;
	device->DriverContext = driver_context;
	device->Port = 0;
    exos_tree_add_child_path(&device->TreeNode, "/dev");
}


io_error_t exos_io_open(io_entry_t *io, const char *path, io_flags_t flags)
{
	EXOS_TREE_NODE *node = exos_tree_find_path(NULL, &path);
	io_error_t res = IO_ERROR_UNKNOWN;
	if (node != NULL && node->Type == EXOS_TREE_NODE_DEVICE)
	{
		io_tree_device_t *dev = (io_tree_device_t *)node;
		exos_io_create(io, dev->Driver, dev->DriverContext, dev->Port);

		const io_driver_t *driver= io->Driver;
		ASSERT(driver != NULL && driver->Open != NULL, KERNEL_ERROR_NULL_POINTER);
		res = driver->Open(io, path, flags);
	}
	else res = IO_ERROR_DEVICE_NOT_FOUND;

	return res;
}


void exos_io_create(io_entry_t *io, const io_driver_t *driver, void *driver_context, unsigned port)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	*io = (io_entry_t) {
#ifdef DEBUG
		.Node = (node_t) { .Type = EXOS_NODE_IO_ENTRY },
#endif
		.Driver = driver, .DriverContext = driver_context, .Port = port };

	exos_event_create(&io->InputEvent);
	exos_event_create(&io->OutputEvent);
}



void exos_io_set_timeout(io_entry_t *io, unsigned long timeout)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	io->Timeout = timeout;
}

int exos_io_read(io_entry_t *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

//	if (io->Flags & EXOS_IOF_WAIT)
		exos_event_wait(&io->InputEvent, io->Timeout);

	const io_driver_t *driver = io->Driver;
	return driver->Read(io, buffer, length);
}

int exos_io_write(io_entry_t *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const io_driver_t *driver = io->Driver;
   	int done = 0;
	if (length > 0)
	{
//		if (io->Flags & EXOS_IOF_WAIT)
		{
			while(length > 0)
			{
				exos_event_wait(&io->OutputEvent, io->Timeout);

				int done2 = driver->Write(io, buffer + done, length);
				if (done2 < 0) return -1;
				if (done2 == 0)
					break;

				done += done2;
				length -= done2;
			}
		}
//		else
//		{
//			done = driver->Write(io, buffer, length);
//		}
	}
	return done;
}

int exos_io_sync(io_entry_t *io)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const io_driver_t *driver = io->Driver;
	if (driver->Sync != NULL)
		return driver->Sync(io);

	return -1;
}



