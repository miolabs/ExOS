#ifndef NET_ARP_H
#define NET_ARP_H

#include "adapter.h"
#include "ip.h"

typedef struct __attribute__((__packed__))
{
	net16_t htype;
	net16_t ptype;
	unsigned char hlen, plen;
	net16_t oper;
	hw_addr_t sha;
	ip_addr_t spa;
	hw_addr_t	tha;
	ip_addr_t tpa;
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
void net_arp_input(net_adapter_t *adapter, ARP_HEADER *arp);
ARP_HEADER *net_arp_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *output, hw_addr_t *destination);
int net_arp_send_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *output);

int net_arp_obtain_hw_addr(net_adapter_t *adapter, ip_addr_t *ip, hw_addr_t *mac);
int net_arp_set_hw_addr(ip_addr_t *ip, hw_addr_t *mac);

#endif // NET_ARP_H
