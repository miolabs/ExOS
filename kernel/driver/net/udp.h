#ifndef NET_UDP_H
#define NET_UDP_H

#include "net.h"
#include "ip.h"
#include "mbuf.h"
#include "adapter.h"

typedef struct
{
	NET16_T SourcePort;
	NET16_T DestinationPort;
	NET16_T Length;
	NET16_T Checksum;
} UDP_HEADER;

int net_udp_input(NET_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip);
int net_udp_send(NET_ADAPTER *driver, IP_ENDPOINT *destination, unsigned short source_port, unsigned short dest_port, NET_MBUF *data);
unsigned short net_udp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset);

#endif // NET_UDP_H
