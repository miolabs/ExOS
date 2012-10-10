#include "io.h"
#include <kernel/panic.h>
#include <kernel/tree.h>

void __io_initialize()
{
	__tree_initialize();
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

	exos_event_create(&io->InputEvent, EXOS_EVENT_AUTO_RESET);
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

