#ifndef NET_TCP_H
#define NET_TCP_H

#include "net.h"
#include "ip.h"
#include "mbuf.h"
#include "adapter.h"

typedef struct __attribute__((__packed__))
{
	unsigned FIN:1;
	unsigned SYN:1;
	unsigned RST:1;
	unsigned PSH:1;
	unsigned ACK:1;
	unsigned URG:1;
	unsigned ECE:1;
	unsigned CWR:1;
} TCP_FLAGS;


typedef struct __attribute__((__packed__))
{
	net16_t SourcePort;
	net16_t DestinationPort;
	net32_t Sequence;
	net32_t Ack;
	struct __attribute__((__packed__))
	{
		unsigned Reserved:4;
		unsigned DataOffset:4;
	};
	TCP_FLAGS Flags;
	net16_t WindowSize;
	net16_t Checksum;
	net16_t UrgentPtr;
	unsigned char Options[0];
} TCP_HEADER;

int net_tcp_input(net_adapter_t *adapter, eth_header_t *buffer, IP_HEADER *ip);
unsigned short net_tcp_checksum(ip_addr_t *source_ip, ip_addr_t *dest_ip, net_mbuf_t *mbuf, int offset);

#endif // NET_TCP_H

