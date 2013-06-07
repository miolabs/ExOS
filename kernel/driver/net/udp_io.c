#include "udp_io.h"
#include <kernel/panic.h>
#include <string.h>

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);
static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _close(NET_IO_ENTRY *io, EXOS_IO_STREAM_BUFFERS *buffers);

static const NET_PROTOCOL_DRIVER _udp_driver = {
	.IO = { .Read = _read, .Write = _write },
	.Bind = _bind, .Receive = _receive, .Send = _send, .Close = _close };

static EXOS_LIST _entries;	// udp bound io entries
static EXOS_MUTEX _entries_mutex;

void __udp_io_initialize()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
}

UDP_IO_ENTRY *__udp_io_find_io(NET_ADAPTER *adapter, unsigned short port)
{
	UDP_IO_ENTRY *found = NULL;
	exos_mutex_lock(&_entries_mutex);
	FOREACH(node, &_entries)
	{
		UDP_IO_ENTRY *io = (UDP_IO_ENTRY *)node;
		if (io->LocalPort == port
			&& (io->Adapter == NULL || io->Adapter == adapter))
		{
			found = io;
		}
	}
	exos_mutex_unlock(&_entries_mutex);
	return found;
}

void net_udp_io_create(UDP_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	net_io_create((NET_IO_ENTRY *)io, &_udp_driver, NET_IO_DATAGRAM, flags);
	
	io->Adapter = NULL;
	io->LocalPort = 0;
	exos_fifo_create(&io->Incoming, &io->InputEvent);
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	UDP_IO_ENTRY *io = (UDP_IO_ENTRY *)socket;
	IP_PORT_ADDR *local = (IP_PORT_ADDR *)addr;

	NET_ADAPTER *adapter = net_adapter_find(local->Address);
	if (io->Adapter != NULL && io->Adapter != adapter)
		return -1; // already bound to another adapter

	// check if already bound
    exos_mutex_lock(&_entries_mutex);

	int error = 0;
	UDP_IO_ENTRY *existing = NULL;
	FOREACH(node, &_entries)
	{
		UDP_IO_ENTRY *i = (UDP_IO_ENTRY *)node;
		if (i != io &&
			i->Adapter == adapter && 
			i->LocalPort == local->Port)
		{
			existing = i;
			break;
		}
	}
	if (existing != NULL)
	{
		error = -2;	// port is bound to another socket
	}
	else
	{
		io->LocalPort = local->Port;
		io->Adapter = adapter;
		
		if (NULL == list_find_node(&_entries, (EXOS_NODE *)io))
			list_add_tail(&_entries, (EXOS_NODE *)io);
	}

	exos_mutex_unlock(&_entries_mutex);
	return error;
}

static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr)
{
	IP_PORT_ADDR *ip = (IP_PORT_ADDR *)addr;
	UDP_IO_ENTRY *io = (UDP_IO_ENTRY *)socket;

	NET_BUFFER *packet = (NET_BUFFER *)exos_fifo_dequeue(&io->Incoming);
	if (packet != NULL)
	{
		IP_HEADER *ip_hdr = (IP_HEADER *)((void *)packet->Buffer + sizeof(ETH_HEADER));
		UDP_HEADER *udp_hdr = (UDP_HEADER *)net_ip_get_payload(ip_hdr, NULL);

		int fit = packet->Length > length ? length : packet->Length; 
		memcpy(buffer, packet->Buffer + packet->Offset, fit);

		if (ip != NULL)
			*ip = (IP_PORT_ADDR) { .Address = ip_hdr->SourceIP, .Port = NTOH16(udp_hdr->SourcePort) };

		net_adapter_discard_input_buffer(packet);
		return fit;
	}
	return -1;
}

static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr)
{
	IP_PORT_ADDR *ip = (IP_PORT_ADDR *)addr;
	UDP_IO_ENTRY *io = (UDP_IO_ENTRY *)socket;

	NET_MBUF msg;
	net_mbuf_init(&msg, buffer, 0, length);
	IP_ENDPOINT ep = (IP_ENDPOINT) { .IP = ip->Address };

	if (ep.IP.Value == -1)
	{
		int done = 0;
		net_adapter_list_lock();
		NET_ADAPTER *adapter = NULL;
		while(net_adapter_enum(&adapter))
		{
			if (adapter->Speed != 0)
			{
				ep.IP.Value = adapter->IP.Value | ~adapter->NetMask.Value;
				if (net_ip_resolve(adapter, &ep))
					done = net_udp_send(adapter, &ep, io->LocalPort, ip->Port, &msg);
			}
		}
		net_adapter_list_unlock();
		return done;
	}
	else
	{
		NET_ADAPTER *adapter = net_adapter_find(ip->Address);
		if (adapter != NULL)
		{
			if (adapter->Speed == 0) return -1;	// down

			if (net_ip_resolve(adapter, &ep))
			{
				return net_udp_send(adapter, &ep, io->LocalPort, ip->Port, &msg);
			}
		}
	}
	return -1;
}

static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif
	
	return _receive((NET_IO_ENTRY *)io, buffer, length, NULL);
}

static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io->Type != EXOS_IO_SOCKET)
		kernel_panic(KERNEL_ERROR_IO_TYPE_MISMATCH);
#endif

	return -1;
}

static int _close(NET_IO_ENTRY *io, EXOS_IO_STREAM_BUFFERS *buffers)
{
	exos_mutex_lock(&_entries_mutex);

	UDP_IO_ENTRY *found = NULL;
	FOREACH(node, &_entries)
	{
		if (io == (NET_IO_ENTRY *)node)
		{
			found = (UDP_IO_ENTRY *)io;
			break;
		}
	}
	if (found != NULL)
		list_remove((EXOS_NODE *)found);

	exos_mutex_unlock(&_entries_mutex);

	if (buffers != NULL)
		*buffers = (EXOS_IO_STREAM_BUFFERS) { .RcvBuffer = NULL, .SndBuffer = NULL };

	return 0;
}





