#include "net_io.h"
#include <kernel/panic.h>

int net_io_bind(NET_IO_ENTRY *io, void *local)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)io->Driver;
	return protocol->Bind(io, local);
}

int net_io_receive(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	
	if (io->Flags & EXOS_IOF_WAIT)
		exos_event_wait(&io->InputEvent, io->Timeout);

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)io->Driver;
	return protocol->Receive(io, buffer, length, remote);
}

int net_io_send(NET_IO_ENTRY *io, void *buffer, unsigned long length, void *remote)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)io->Driver;
	return protocol->Send(io, buffer, length, remote);
}


