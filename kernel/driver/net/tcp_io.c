#include "tcp_io.h"
#include "tcp_service.h"
#include <kernel/panic.h>

static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _listen(NET_IO_ENTRY *socket);
static int _accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, EXOS_IO_STREAM_BUFFERS *buffers);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const NET_PROTOCOL_DRIVER _tcp_driver = {
	.IO = { .Read = _read, .Write = _write }, 
	.Bind = _bind, .Listen = _listen, .Accept = _accept };

void __tcp_io_initialize()
{
	net_tcp_service_start();
}

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	net_io_create((NET_IO_ENTRY *)io, &_tcp_driver, NET_IO_STREAM, flags);

	io->RcvBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL, .Size = 0 };
	io->SndBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL, .Size = 0 };

	io->BufferSize = 32;	// FIXME

	io->State = TCP_STATE_CLOSED;
	exos_mutex_create(&io->Mutex);
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	IP_PORT_ADDR *local = (IP_PORT_ADDR *)addr;
	//ETH_ADAPTER *adapter = net_adapter_find(local->Address);

	int done = net_tcp_bind(io, local->Port);
	return done ? 0 : -1;
}

static int _listen(NET_IO_ENTRY *socket)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;

	int done = 0;
	if (io->LocalPort != 0)
	{
		exos_fifo_create(&io->AcceptQueue, &io->InputEvent);

		io->State = TCP_STATE_LISTEN; 
		return 0;
	}
	return -1;
}

static int _accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, EXOS_IO_STREAM_BUFFERS *buffers)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	if (io->State == TCP_STATE_LISTEN)
	{
		TCP_INCOMING_CONN *conn = (TCP_INCOMING_CONN *)exos_fifo_wait(&io->AcceptQueue, io->Timeout);
		if (conn != NULL)
		{
			int done = net_tcp_accept((TCP_IO_ENTRY *)conn_socket, buffers, conn);
			return done ? 0 : -1;
		}
	}
	return -1;
}

static int _connect(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers, IP_PORT_ADDR *addr)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;

	// TODO
	return -1;
}

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	
	TCP_IO_ENTRY *tcp_io = (TCP_IO_ENTRY *)io;
	return exos_io_buffer_read(&tcp_io->RcvBuffer, buffer, length);
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif

	TCP_IO_ENTRY *tcp_io = (TCP_IO_ENTRY *)io;
	int done = exos_io_buffer_write(&tcp_io->SndBuffer, (void *)buffer, length);

	tcp_io->SndFlags.PSH = 1;
	net_tcp_service(tcp_io, 0);

	return done;
}





