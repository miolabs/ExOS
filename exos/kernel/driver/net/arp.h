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
} arp_header_t;

typedef enum
{
	ARP_HTYPE_ETHERNET = 1
} arp_hw_t;

typedef enum
{
	ARP_PTYPE_IPV4 = 0x0800
} arp_protocol_t;

typedef enum
{
	ARP_OPER_REQUEST = 1,
	ARP_OPER_REPLY = 2,
} arp_operation_t;

// prototypes
void net_arp_initialize();
bool net_arp_input(net_adapter_t *adapter, arp_header_t *arp);
arp_header_t *net_arp_output(net_adapter_t *adapter, net_buffer_t *output, hw_addr_t *destination);
//int net_arp_send_output(net_adapter_t *adapter, net_buffer_t *output);

int net_arp_obtain_hw_addr(net_adapter_t *adapter, ip_addr_t *ip, hw_addr_t *mac);
int net_arp_set_hw_addr(ip_addr_t *ip, hw_addr_t *mac);

#endif // NET_ARP_H
