#include "comm.h"
#include <kernel/panic.h>

//void comm_initialize()
//{
//}

//void comm_io_create(COMM_IO_ENTRY *io, COMM_DEVICE *device, unsigned port/*, EXOS_IO_FLAGS flags*/)
//{
//	ASSERT(io != NULL && device != NULL, KERNEL_ERROR_NULL_POINTER);
//
//	exos_io_create((io_entry_t *)io, /*EXOS_IO_COMM,*/ &_comm_driver/*, flags*/);
//	exos_event_create(&io->SyncEvent);
//	io->Device = device;
//	io->Port = port;
//}

bool comm_io_create_from_path(COMM_IO_ENTRY *io, const char *path, io_flags_t flags)
{
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, &path);
	if (dev_node != NULL && 
		dev_node->TreeNode.Type == EXOS_TREE_NODE_DEVICE)
	{
		io_tree_device_t *dev = (io_tree_device_t *)dev_node;
		exos_io_create(io, dev->Driver, dev->DriverContext, dev->Port);
		// FIXME: flags is ignored
		return true;
	}
	return false;
}

int comm_io_open(COMM_IO_ENTRY *io)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	const io_driver_t *driver = io->Driver;
	ASSERT(driver != NULL && driver->Open != NULL, KERNEL_ERROR_NULL_POINTER);
	io_error_t res = driver->Open(io, NULL, IOF_NONE);	// FIXME: name? flags?
	return (res == IO_OK) ? 0 : -1;
}

void comm_io_close(COMM_IO_ENTRY *io)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_io_sync(io);
	const io_driver_t *driver = io->Driver;
	ASSERT(driver != NULL && driver->Close != NULL, KERNEL_ERROR_NULL_POINTER);
	driver->Close(io);
}

bool comm_io_get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *pvalue)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pvalue != NULL, KERNEL_ERROR_NULL_POINTER);

	const io_driver_t *driver = io->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	bool done = false;
//	if (driver->GetAttr != NULL)
//		done = driver->GetAttr(io, attr, pvalue);
	return done;
}

bool comm_io_set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *pvalue)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(pvalue != NULL, KERNEL_ERROR_NULL_POINTER);

	const io_driver_t *driver = io->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
	bool done = false;
//	if (driver->SetAttr != NULL)
//		done = driver->SetAttr(io, attr, pvalue);
	return done;
}

//static int _sync(io_entry_t *io)
//{
//	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
////	ASSERT(io->Type == EXOS_IO_COMM, KERNEL_ERROR_IO_TYPE_MISMATCH);
//
//	COMM_IO_ENTRY *comm = (COMM_IO_ENTRY *)io;
//	int res = exos_event_wait(&comm->SyncEvent, io->Timeout);	// NOTE: timeout 0 will never timeout
//	return res;
//}




