#include "tcp_io.h"
#include <kernel/panic.h>

static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const NET_PROTOCOL_DRIVER _tcp_driver = {
	.IO = { .Read = _read, .Write = _write }, 
	.Bind = _bind };

static EXOS_LIST _entries;	// bound io entries
static EXOS_MUTEX _entries_mutex;

void __tcp_io_initialize()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
}

TCP_IO_ENTRY *__tcp_io_find_io(ETH_ADAPTER *adapter, unsigned short local_port, IP_ADDR src_ip, unsigned short src_port)
{
	TCP_IO_ENTRY *found = NULL;
	exos_mutex_lock(&_entries_mutex);
	FOREACH(node, &_entries)
	{
		TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)node;
		if ((io->Adapter == NULL || io->Adapter == adapter) &&
			io->LocalPort == local_port &&
			(io->RemoteEP.IP.Value == 0 || (io->RemoteEP.IP.Value == src_ip.Value && io->RemotePort == src_port)))
			found = io;
	}
	exos_mutex_unlock(&_entries_mutex);
	return found;
}

void __tcp_io_remove_io(TCP_IO_ENTRY *io)
{
	exos_mutex_lock(&_entries_mutex);
#ifdef DEBUG
	if (!list_find_node(&_entries, (EXOS_NODE *)io))
		kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
	list_remove((EXOS_NODE *)io);
	exos_mutex_unlock(&_entries_mutex);
}

void net_tcp_create_io(TCP_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_SOCKET, (const EXOS_IO_DRIVER *)&_tcp_driver, flags);
	
	io->Adapter = NULL;
	// TODO
	io->State = TCP_STATE_CLOSED;
	exos_fifo_create(&io->Incoming, &io->InputEvent);
}

int net_tcp_listen(TCP_IO_ENTRY *io)
{
	if (io->Type != EXOS_IO_SOCKET) return -1;
#ifdef DEBUG
	if (io->Driver == NULL) kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int done = 0;
	if (io->State == TCP_STATE_BOUND)
	{
		io->RemotePort = 0;
		io->RemoteEP = (IP_ENDPOINT) { };
		io->LocalWindowSize = 1;	// FIXME
		io->RemoteWindowSize = 0;

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

	int done = 0;
	exos_mutex_lock(&_entries_mutex);
	TCP_IO_ENTRY *existing = __tcp_io_find_io(adapter, local->Port, IP_ENDPOINT_BROADCAST->IP, 0);
	if (existing == NULL)
	{
		io->LocalPort = local->Port;
		io->State = TCP_STATE_BOUND;
		list_add_tail(&_entries, (EXOS_NODE *)io);
		done = 1;
	}
	exos_mutex_unlock(&_entries_mutex);

	return done ? 0 : -1;
}

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	
	return -1;
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif

	return -1;
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




