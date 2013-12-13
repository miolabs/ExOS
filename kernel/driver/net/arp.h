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
void net_arp_initialize();
void net_arp_input(NET_ADAPTER *adapter, ARP_HEADER *arp);
ARP_HEADER *net_arp_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, HW_ADDR *destination);
int net_arp_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output);

int net_arp_obtain_hw_addr(NET_ADAPTER *adapter, IP_ADDR *ip, HW_ADDR *mac);
int net_arp_set_hw_addr(IP_ADDR *ip, HW_ADDR *mac);

#endif // NET_ARP_H
