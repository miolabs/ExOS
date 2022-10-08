#ifndef NET_UDP_H
#define NET_UDP_H

#include "net.h"
#include "ip.h"
#include "mbuf.h"
#include "adapter.h"

typedef struct
{
	net16_t SourcePort;
	net16_t DestinationPort;
	net16_t Length;
	net16_t Checksum;
} udp_header_t;

#ifdef EXOS_OLD
typedef udp_header_t UDP_HEADER;
#endif

int net_udp_input(net_adapter_t *adapter, eth_header_t *buffer, IP_HEADER *ip);
int net_udp_send(net_adapter_t *driver, const IP_ENDPOINT *destination, unsigned short source_port, unsigned short dest_port, net_mbuf_t *data);
unsigned short net_udp_checksum(const ip_addr_t *source_ip, const ip_addr_t *dest_ip, net_mbuf_t *mbuf, int offset);

#endif // NET_UDP_H
