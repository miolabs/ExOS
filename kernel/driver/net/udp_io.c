#include "udp_io.h"
#include <kernel/panic.h>
#include <string.h>

static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const NET_PROTOCOL_DRIVER _udp_driver = {
	.IO = { .Read = _read, .Write = _write },
	.Bind = _bind, .Receive = _receive, .Send = _send };

static EXOS_LIST _entries;	// udp bound io entries
static EXOS_MUTEX _entries_mutex;

void __udp_io_initialize()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
}

UDP_IO_ENTRY *__udp_io_find_io(ETH_ADAPTER *adapter, unsigned short port)
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

void net_udp_create_io(UDP_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_SOCKET, (const EXOS_IO_DRIVER *)&_udp_driver, flags);
	
	io->Adapter = NULL;
	io->LocalPort = 0;
	exos_fifo_create(&io->Incoming, &io->InputEvent);
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	UDP_IO_ENTRY *io = (UDP_IO_ENTRY *)socket;
	IP_PORT_ADDR *local = (IP_PORT_ADDR *)addr;

	ETH_ADAPTER *adapter = net_adapter_find(local->Address);
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

	ETH_INPUT_BUFFER *packet = (ETH_INPUT_BUFFER *)exos_fifo_dequeue(&io->Incoming);
	if (packet != NULL)
	{
		IP_HEADER *ip_hdr = (IP_HEADER *)((void *)packet->Buffer + sizeof(ETH_HEADER));
		unsigned short udp_length;
		UDP_HEADER *udp_hdr = net_ip_get_payload(ip_hdr, &udp_length);
		
		void *payload = (void *)udp_hdr + sizeof(UDP_HEADER);
		int payload_length = (int)udp_length - sizeof(UDP_HEADER);

		int fit = payload_length > length ? length : payload_length; 
		memcpy(buffer, payload, fit);

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
		net_adapter_list_lock();
		ETH_ADAPTER *adapter = NULL;
		while(net_adapter_enum(&adapter))
		{
			if (adapter->Speed != 0)
			{
				ep.IP.Value = adapter->IP.Value | ~adapter->NetMask.Value;
				if (net_ip_resolve(adapter, &ep))
					net_udp_send(adapter, &ep, io->LocalPort, ip->Port, &msg);
			}
		}
		net_adapter_list_unlock();
		return 0;
	}
	else
	{
		ETH_ADAPTER *adapter = net_adapter_find(ip->Address);
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





