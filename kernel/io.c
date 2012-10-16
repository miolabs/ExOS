#include "io.h"
#include <kernel/panic.h>
#include <kernel/tree.h>
#include <comm/comm.h>

#ifndef EXOS_NO_NET
#include <net/net.h>
#endif

void __io_initialize()
{
	__tree_initialize();
	comm_initialize();
#ifndef EXOS_NO_NET
	net_initialize();
#endif
}

void exos_io_create(EXOS_IO_ENTRY *io, EXOS_IO_TYPE type, const EXOS_IO_DRIVER *driver, EXOS_IO_FLAGS flags)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	
	*io = (EXOS_IO_ENTRY) {
#ifdef DEBUG
		.Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_ENTRY },
#endif
		.Type = type, .Driver = driver, .Flags = flags };
	exos_event_create(&io->InputEvent);
	exos_event_create(&io->OutputEvent);
}

void exos_io_set_flags(EXOS_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	io->Flags = flags;
}

void exos_io_set_timeout(EXOS_IO_ENTRY *io, unsigned long timeout)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	io->Timeout = timeout;
}

int exos_io_read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (io->Flags & EXOS_IOF_WAIT)
		exos_event_wait(&io->InputEvent, io->Timeout);

	const EXOS_IO_DRIVER *driver = io->Driver;
	return driver->Read(io, buffer, length);
}

int exos_io_write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const EXOS_IO_DRIVER *driver = io->Driver;
	if (io->Flags & EXOS_IOF_WAIT)
	{
		int done = 0;
		while(length > 0)
		{
			exos_event_wait(&io->OutputEvent, io->Timeout);
	
			int done2 = driver->Write(io, buffer + done, length);
			if (done2 < 0) return -1;
			if (done2 == 0) break;
	
			done += done2;
			length -= done2;
		}
		return done;
	}
	else
	{
		return driver->Write(io, buffer, length);
	}
}