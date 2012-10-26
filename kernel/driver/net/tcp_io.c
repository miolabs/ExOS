#include "tcp_io.h"
#include "tcp_service.h"
#include <kernel/panic.h>

static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const NET_PROTOCOL_DRIVER _tcp_driver = {
	.IO = { .Read = _read, .Write = _write }, 
	.Bind = _bind };

void __tcp_io_initialize()
{
	net_tcp_service_start();
}

void net_tcp_io_create(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags, void *rcv_buffer, unsigned short rcv_length, void *snd_buffer, unsigned short snd_length)
{
	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_SOCKET, (const EXOS_IO_DRIVER *)&_tcp_driver, flags);
	
	io->Adapter = NULL;
	io->State = TCP_STATE_CLOSED;
	
	exos_io_buffer_create(&io->RcvBuffer, rcv_buffer, rcv_length);
	io->RcvBuffer.NotEmptyEvent = &io->InputEvent;

	exos_io_buffer_create(&io->SndBuffer, snd_buffer, snd_length);
	io->SndBuffer.NotFullEvent = &io->OutputEvent;

	exos_mutex_create(&io->Mutex);
}

int net_tcp_listen(TCP_IO_ENTRY *io)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int done = 0;
	if (io->LocalPort != 0)
	{
		io->RemotePort = 0;
		io->RemoteEP = (IP_ENDPOINT) { };

		io->SndAck = io->SndNext = 0; // FIXME: use random for security
		io->State = TCP_STATE_LISTEN; 
		return 0;
	}
	return -1;
}

int net_tcp_accept(TCP_IO_ENTRY *io)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (io->Flags & EXOS_IOF_WAIT)
	exos_event_wait(&io->OutputEvent, io->Timeout);

	return -1;
}

int net_tcp_connect(TCP_IO_ENTRY *io, IP_PORT_ADDR *addr)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	return -1;
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)socket;
	IP_PORT_ADDR *local = (IP_PORT_ADDR *)addr;
	ETH_ADAPTER *adapter = net_adapter_find(local->Address);

	int done = net_tcp_bind(io, local->Port, adapter);
	return done ? 0 : -1;
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

//int net_tcp_append(TCP_PCB *pcb, NET_MBUF *mbuf, int push)
//{
//	// FIXME: we should disable processing
//	
//	if (push) pcb->SndFlags.PSH = 1;
//
//	if (pcb->SndBuffer != NULL)
//	{
//		net_mbuf_append(pcb->SndBuffer, mbuf);
//	}
//	else
//	{
//		pcb->SndBuffer = mbuf;
//	}
//	//net_wakeup();
//}




