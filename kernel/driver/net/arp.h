#ifndef NET_ARP_H
#define NET_ARP_H

#include "adapter.h"

typedef struct __attribute__((__packed__))
{
	NET16_T htype;
	NET16_T ptype;
	unsigned char hlen, plen;
	NET16_T oper;
	HW_ADDR sha;
	IP_ADDR spa;
	HW_ADDR	tha;
	IP_ADDR tpa;
} ARP_HEADER;

typedef enum
{
	ARP_HTYPE_ETHERNET = 1
} ARP_HTYPE;

typedef enum
{
	ARP_PTYPE_IPV4 = 0x0800
} ARP_PTYPE;

typedef enum
{
	ARP_OPER_REQUEST = 1,
	ARP_OPER_REPLY = 2,
} ARP_OPER;

// prototypes
void net_arp_input(ETH_ADAPTER *adapter, ARP_HEADER *arp);
ARP_HEADER *net_arp_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output, HW_ADDR *destination);
int net_arp_send_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output);

#endif // NET_ARP_H
