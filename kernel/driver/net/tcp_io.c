#include "tcp_io.h"
#include "tcp_service.h"
#include <net/arp.h>
#include <kernel/panic.h>

static int _connect(NET_IO_ENTRY *socket, void *addr, const EXOS_IO_STREAM_BUFFERS *buffers);
static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _listen(NET_IO_ENTRY *socket);
static int _accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, const EXOS_IO_STREAM_BUFFERS *buffers);
static int _close(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);
static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);
static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote);

static const NET_PROTOCOL_DRIVER _tcp_driver = {
	.IO = { .Read = _read, .Write = _write }, 
	.Connect = _connect, 
	.Bind = _bind, .Listen = _listen, .Accept = _accept,
	.Receive = _receive, .Send = _send,
	.Close = _close };

void __tcp_io_initialize()
{
	net_tcp_service_start();
}

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	net_io_create((NET_IO_ENTRY *)io, &_tcp_driver, NET_IO_STREAM, flags);

	io->RcvBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL, .Size = 0 };
	io->SndBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL, .Size = 0 };

	io->BufferSize = 32;	// FIXME: use pre-defined initial value

	io->State = TCP_STATE_CLOSED;

	exos_mutex_create(&io->Mutex);
	exos_event_create(&io->CloseEvent);
}

static int _connect(NET_IO_ENTRY *socket, void *addr, const EXOS_IO_STREAM_BUFFERS *buffers)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	IP_PORT_ADDR *remote = (IP_PORT_ADDR *)addr;

	int done = 0;
	if (io->State == TCP_STATE_CLOSED)
	{
		done = net_tcp_connect(io, buffers, remote);
	}
	return done ? 0 : -1;
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	IP_PORT_ADDR *local = (IP_PORT_ADDR *)addr;

	int done;
	if (io->State == TCP_STATE_CLOSED)
	{
		//ETH_ADAPTER *adapter = net_adapter_find(local->Address);
		io->LocalPort = local->Port;
		done = 1;
	}
	return done ? 0 : -1;
}

static int _listen(NET_IO_ENTRY *socket)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;

	int done = 0;
	if (io->State == TCP_STATE_CLOSED)
	{
		done = net_tcp_listen(io, io->LocalPort);
	}
	return done ? 0 : -1;
}

static int _accept(NET_IO_ENTRY *socket, NET_IO_ENTRY *conn_socket, const EXOS_IO_STREAM_BUFFERS *buffers)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	if (io->State == TCP_STATE_LISTEN)
	{
		TCP_INCOMING_CONN *conn = (TCP_INCOMING_CONN *)exos_fifo_wait(&io->AcceptQueue, io->Timeout);
		if (conn != NULL)
		{
			TCP_IO_ENTRY *conn_io = (TCP_IO_ENTRY *)conn_socket;
			if (conn_io->State == TCP_STATE_LISTEN)
				net_tcp_close(conn_io);

			int done = net_tcp_accept(conn_io, buffers, conn);
			return done ? 0 : -1;
		}
	}
	return -1;
}

static int _close(NET_IO_ENTRY *socket, EXOS_IO_STREAM_BUFFERS *buffers)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	if (buffers != NULL)
	{
		*buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = io->RcvBuffer.Buffer, .RcvBufferSize = io->RcvBuffer.Size,
			.SndBuffer = io->SndBuffer.Buffer, .SndBufferSize = io->SndBuffer.Size };
	}
	int done = net_tcp_close(io);
	return done ? 0 : -1;
}

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	
	TCP_IO_ENTRY *tcp_io = (TCP_IO_ENTRY *)io;
	int done = exos_io_buffer_read(&tcp_io->RcvBuffer, buffer, length);
	 
	if (done == 0 && tcp_io->State != TCP_STATE_ESTABLISHED) 
		done = -1;

//	if (done > 0) 
//		net_tcp_service(tcp_io, 0);
	return done;
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif

	TCP_IO_ENTRY *tcp_io = (TCP_IO_ENTRY *)io;
	int done = (tcp_io->State != TCP_STATE_ESTABLISHED) ? -1 :
		exos_io_buffer_write(&tcp_io->SndBuffer, (void *)buffer, length);

	tcp_io->SndFlags.PSH = 1;	// FIXME: use when needed (requested or buffer full)
	net_tcp_service(tcp_io, 0);

	return done;
}

static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	
	IP_PORT_ADDR *pipp = (IP_PORT_ADDR *)remote;
	if (pipp != NULL)
	{
		*pipp = (IP_PORT_ADDR) { .Address = io->RemoteEP.IP, .Port = io->RemotePort };
	}
	return _read((EXOS_IO_ENTRY *)socket, buffer, length); 
}

static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *remote)
{
	// NOTE: remote EP is ignored
	return _write((EXOS_IO_ENTRY *)socket, buffer, length); 
}





