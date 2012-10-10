#ifndef NET_ICMP_H
#define NET_ICMP_H

#include "ip.h"

typedef struct
{
	unsigned char Type;
	unsigned char Code;
	NET16_T Checksum;
	NET16_T Id;
	NET16_T Sequence;
	unsigned char Data[0];
} ICMP_HEADER;

typedef enum
{
	ICMP_TYPE_ECHO_REPLY = 0,
	ICMP_TYPE_ECHO_REQUEST = 8,
} ICMP_TYPE;


// prototypes
void net_icmp_input(ETH_ADAPTER *adapter, ETH_HEADER *eth, IP_HEADER *ip);

#endif // NET_ICMP_H
