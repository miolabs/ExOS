#include "net_io.h"
#include <kernel/panic.h>

void net_io_create(NET_IO_ENTRY *socket, const NET_PROTOCOL_DRIVER *driver, NET_IO_TYPE protocol, EXOS_IO_FLAGS flags)
{
	exos_io_create((EXOS_IO_ENTRY *)socket, EXOS_IO_SOCKET, (const EXOS_IO_DRIVER *)driver, flags);
	socket->Adapter = NULL;
	socket->ProtocolType = protocol;
}

int net_io_bind(NET_IO_ENTRY *socket, void *local)
{
	if (socket->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Bind(socket, local);
}

int net_io_receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote)
{
	if (socket->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	
	if (socket->Flags & EXOS_IOF_WAIT)
		exos_event_wait(&socket->InputEvent, socket->Timeout);

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Receive(socket, buffer, length, remote);
}

int net_io_send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote)
{
	if (socket->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Send(socket, buffer, length, remote);
}

int net_io_listen(NET_IO_ENTRY *socket)
{
	if (socket->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Listen(socket);
}

int net_io_accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, const EXOS_IO_STREAM_BUFFERS *buffers)
{
	if (socket == NULL || socket->Type != EXOS_IO_SOCKET) return -1;
	if (conn_socket == NULL || conn_socket->Type != EXOS_IO_SOCKET) return -1;

#ifdef DEBUG
	if (socket->Driver == NULL || conn_socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	if (socket->Driver != conn_socket->Driver) return -1;

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Accept(socket, conn_socket, buffers);
}

int net_io_close(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers)
{
	if (socket->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (socket->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const NET_PROTOCOL_DRIVER *protocol = (const NET_PROTOCOL_DRIVER *)socket->Driver;
	return protocol->Close(socket, buffers);
}
