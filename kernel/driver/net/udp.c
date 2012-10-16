// IP Stack. User Datagram Protocol Support
// by Miguel Fides

#include "udp.h"
#include "mbuf.h"
#include "net_io.h"
#include <kernel/panic.h>
#include <string.h>

static int _bind(NET_IO_ENTRY *socket, void *addr);
static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _send(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr);
static int _read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
static int _write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length);

static const NET_PROTOCOL_DRIVER _udp_driver = {
	.IO = { .Read = _read, .Write = _write  },
	.Bind = _bind, .Receive = _receive, .Send = _send };

static EXOS_LIST _entries;	// udp bound io entries
static EXOS_MUTEX _entries_mutex;

void net_udp_initialize()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
}

int net_udp_input(ETH_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip)
{
	unsigned short msg_length;
	UDP_HEADER *udp = (UDP_HEADER *)net_ip_get_payload(ip, &msg_length);
	unsigned short udp_length = NTOH16(udp->Length);

	int queued = 0;
	if (udp_length == msg_length)
	{
		NET_MBUF msg;
		net_mbuf_init(&msg, udp, 0, udp_length);
		unsigned short checksum = NTOH16(udp->Checksum);	// a value of zero means that checksum is not used (optional)
		if (checksum == 0 || 
			0 == net_udp_checksum(&ip->SourceIP, &ip->DestinationIP, &msg, 0))
		{
			unsigned short port = NTOH16(udp->DestinationPort);

			exos_mutex_lock(&_entries_mutex);
			FOREACH(node, &_entries)
			{
				NET_IO_ENTRY *io = (NET_IO_ENTRY *)node;
				if (io->LocalPort == port
					&& (io->Adapter == NULL || io->Adapter == adapter))
				{
					ETH_INPUT_BUFFER *packet = net_adapter_alloc_input_buffer(adapter, buffer);
					exos_fifo_queue(&io->Incoming, (EXOS_NODE *)packet);
					queued = 1;
					//exos_event_set(&io->InputEvent);
				}
			}
			exos_mutex_unlock(&_entries_mutex);
		}
	}
	return queued;
}

int net_udp_send(ETH_ADAPTER *adapter, IP_ENDPOINT *destination, unsigned short source_port, unsigned short dest_port, NET_MBUF *data)
{
	EXOS_EVENT completed_event;
	exos_event_create(&completed_event);
	ETH_OUTPUT_BUFFER resp = (ETH_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };

	UDP_HEADER *udp = (UDP_HEADER *)net_ip_output(adapter, &resp, sizeof(UDP_HEADER), destination, IP_PROTOCOL_UDP);
	if (udp != NULL)
	{
		int udp_payload = net_mbuf_length(data);
		// FIXME: limit payload length
		int udp_length = udp_payload + sizeof(UDP_HEADER);
	
		udp->SourcePort = HTON16(source_port);
		udp->DestinationPort = HTON16(dest_port);
		udp->Length = HTON16(udp_length);
		udp->Checksum = HTON16(0);
	
		net_mbuf_append(&resp.Buffer, data);
	
		int udp_offset = (unsigned char *)udp - (unsigned char *)resp.Buffer.Buffer; 
		unsigned short checksum = net_udp_checksum(&adapter->IP, &destination->IP, &resp.Buffer, udp_offset);
		udp->Checksum = HTON16(checksum);

		int done = net_ip_send_output(adapter, &resp, udp_length);
		if (done)
			exos_event_wait(&completed_event, EXOS_TIMEOUT_NEVER);

		return udp_payload;
	}
	return -1;
}

unsigned short net_udp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset)
{
	int total_length = 0;
	unsigned long sum = 0;
	while(mbuf != NULL)
	{
		int length = mbuf->Length - (mbuf->Offset + offset);
		int word_count = length >> 1;
		NET16_T *data = (NET16_T *)(mbuf->Buffer + mbuf->Offset + offset);
		if (length & 1)
		{
			sum += ((unsigned char *)data)[length - 1] << 8;
		}
		for(int i = 0; i < word_count; i++)
		{
			sum += NTOH16(data[i]);
		}
		total_length += length;
		mbuf = mbuf->Next;
		offset = 0;
	}

	IP_PSEUDO_HEADER pseudo = { *source_ip, *dest_ip, 0, IP_PROTOCOL_UDP, HTON16(total_length) };
	NET16_T *pseudo_words = (NET16_T *)&pseudo;
	for(int i = 0; i < (sizeof(IP_PSEUDO_HEADER) >> 1); i++)
	{
		sum += NTOH16(pseudo_words[i]);
	}
	sum += (sum >> 16);	// add carry to do 1's complement sum
	return (unsigned short)~sum;
}

void net_udp_create_io(NET_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
	exos_io_create((EXOS_IO_ENTRY *)io, EXOS_IO_SOCKET, (const EXOS_IO_DRIVER *)&_udp_driver, flags);
	
	io->Adapter = NULL;
	io->LocalPort = 0;
	exos_fifo_create(&io->Incoming, &io->InputEvent);
}

static int _bind(NET_IO_ENTRY *socket, void *addr)
{
	IP_PORT_ADDR *ip = (IP_PORT_ADDR *)addr;

	ETH_ADAPTER *adapter = net_adapter_find(ip->Address);
	if (socket->Adapter != NULL && socket->Adapter != adapter)
		return -1; // already bound to another adapter

	// check if already bound
    exos_mutex_lock(&_entries_mutex);

	int error = 0;
	NET_IO_ENTRY *existing = NULL;
	FOREACH(node, &_entries)
	{
		NET_IO_ENTRY *s = (NET_IO_ENTRY *)node;
		if (s != socket &&
			s->Adapter == adapter && 
			s->LocalPort == ip->Port)
		{
			existing = s;
			break;
		}
	}
	if (existing != NULL)
	{
		error = -2;	// port is bound to another socket
	}
	else
	{
		socket->LocalPort = ip->Port;
		socket->Adapter = adapter;
		
		if (NULL == list_find_node(&_entries, (EXOS_NODE *)socket))
			list_add_tail(&_entries, (EXOS_NODE *)socket);
	}

	exos_mutex_unlock(&_entries_mutex);
	return error;
}

static int _receive(NET_IO_ENTRY *socket, void *buffer, unsigned long length, void *addr)
{
	IP_PORT_ADDR *ip = (IP_PORT_ADDR *)addr;

	ETH_INPUT_BUFFER *packet = (ETH_INPUT_BUFFER *)exos_fifo_dequeue(&socket->Incoming);
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
					net_udp_send(adapter, &ep, socket->LocalPort, ip->Port, &msg);
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
				return net_udp_send(adapter, &ep, socket->LocalPort, ip->Port, &msg);
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







