// IP Stack. User Datagram Protocol Support
// by Miguel Fides

#include "udp.h"
#include "mbuf.h"
#include "udp_io.h"

int net_udp_input(ETH_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip)
{
	unsigned short msg_length;
	UDP_HEADER *udp = (UDP_HEADER *)net_ip_get_payload(ip, &msg_length);
	unsigned short udp_length = NTOH16(udp->Length);

	if (udp_length == msg_length)
	{
		NET_MBUF msg;
		net_mbuf_init(&msg, udp, 0, udp_length);
		unsigned short checksum = NTOH16(udp->Checksum);	// a value of zero means that checksum is not used (optional)
		if (checksum == 0 || 
			0 == net_udp_checksum(&ip->SourceIP, &ip->DestinationIP, &msg, 0))
		{
			unsigned short port = NTOH16(udp->DestinationPort);
			UDP_IO_ENTRY *io = __udp_io_find_io(adapter, port);
			if (io != NULL)
			{
				ETH_BUFFER *packet = net_adapter_alloc_buffer(adapter, buffer, (void *)udp + sizeof(UDP_HEADER), udp_length - sizeof(UDP_HEADER));
				exos_fifo_queue(&io->Incoming, (EXOS_NODE *)packet);
				return 1;
			}
		}
	}
	return 0;
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









