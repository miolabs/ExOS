#ifndef NET_ICMP_H
#define NET_ICMP_H

#include "ip.h"

typedef struct
{
	unsigned char Type;
	unsigned char Code;
	net16_t Checksum;
	net16_t Id;
	net16_t Sequence;
	unsigned char Data[0];
} ICMP_HEADER;

typedef enum
{
	ICMP_TYPE_ECHO_REPLY = 0,
	ICMP_TYPE_ECHO_REQUEST = 8,
} ICMP_TYPE;


// prototypes
bool net_icmp_input(net_adapter_t *adapter, eth_header_t *eth, ip_header_t *ip);

#endif // NET_ICMP_H
