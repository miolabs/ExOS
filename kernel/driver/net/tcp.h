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
	NET16_T SourcePort;
	NET16_T DestinationPort;
	NET32_T Sequence;
	NET32_T Ack;
	struct __attribute__((__packed__))
	{
		unsigned Reserved:4;
		unsigned DataOffset:4;
	};
	TCP_FLAGS Flags;
	NET16_T WindowSize;
	NET16_T Checksum;
	NET16_T UrgentPtr;
	unsigned char Options[0];
} TCP_HEADER;

typedef enum
{
	TCP_STATE_CLOSED,
	TCP_STATE_LISTEN,
	TCP_STATE_SYN_SENT,
	TCP_STATE_SYN_RECEIVED,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_CLOSE_WAIT,
	TCP_STATE_LAST_ACK,
	TCP_STATE_FIN_WAIT_1,
	TCP_STATE_FIN_WAIT_2,
	TCP_STATE_CLOSING,
	TCP_STATE_TIME_WAIT,
} TCP_STATE;


int net_tcp_input(NET_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip);
unsigned short net_tcp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset);

#endif // NET_TCP_H

