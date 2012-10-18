#ifndef NET_IP_H
#define NET_IP_H

#include "adapter.h"

typedef enum
{
	IP_VER_RESERVED = 0,
	IP_VER_IPv4 = 4,
	IP_VER_ST = 5,
	IP_VER_IPv6 = 6,
} IP_VER;

typedef enum
{
	IP_PROTOCOL_ICMP = 1,
	IP_PROTOCOL_ST = 5,
	IP_PROTOCOL_TCP = 6,
	IP_PROTOCOL_UDP = 17,
	IP_PROTOCOL_UDP_LITE = 136,
} IP_PROTOCOL;

typedef struct __attribute__((__packed__))
{
	struct __attribute__((__packed__))
	{
		unsigned HeaderLength:4;
		IP_VER Version:4;
	};
	unsigned char DiffServ;
	NET16_T TotalLength;
	NET16_T Id;
	NET16_T Fragment;
	unsigned char TTL;
	IP_PROTOCOL Protocol:8;
	NET16_T HeaderChecksum;
	IP_ADDR SourceIP;
	IP_ADDR DestinationIP;
} IP_HEADER;

#define IP_FRAGF_RF		0x8000	// Reserved Fragment
#define IP_FRAGF_DF		0x4000	// Don't Fragment
#define IP_FRAGF_MF		0x2000	// More Fragments
#define IP_FRAGF_OFFSET	0x1fff

typedef struct
{
	IP_ADDR SourceIP;
	IP_ADDR DestinationIP;
	unsigned char Reserved;
	unsigned char Protocol;
	NET16_T TotalLength;
} IP_PSEUDO_HEADER;

typedef struct __attribute__((__packed__))
{
	HW_ADDR MAC;
	unsigned char Reserved;
	union __attribute__((__packed__))
	{
		unsigned char Value;
		struct __attribute__((__packed__))
		{
			unsigned Reserved : 2;
			unsigned DSCP : 6;
		} Bits;
	} DiffServ;
	IP_ADDR IP;
} IP_ENDPOINT;

#define DSCP_CS1	0b001000	// background
#define DSCP_AF11	0b001010	// video
#define DSCP_CS5	0b101000	// video
#define DSCP_AF12	0b001100	// voice
#define DSCP_CS7	0b111000	// voice
#define DSCP_AF41	0b100010	// voice
#define DSCP_EF		0b101110	// voice

extern const IP_ENDPOINT __ep_broadcast;
extern const IP_ADDR __ip_any;

#define IP_ENDPOINT_BROADCAST ((IP_ENDPOINT *)&__ep_broadcast)
#define IP_ADDR_BROADCAST (__ep_broadcast.IP)
#define IP_ADDR_ANY (__ip_any)

typedef struct
{
	IP_ADDR Address;
	unsigned short Port;
} IP_PORT_ADDR;

// prototypes
void net_ip_initialize();
int net_ip_input(ETH_ADAPTER *adapter, ETH_HEADER *eth, IP_HEADER *ip);
void *net_ip_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output, unsigned hdr_size, IP_ENDPOINT *destination, IP_PROTOCOL protocol);
int net_ip_send_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output, unsigned payload);
void *net_ip_get_payload(IP_HEADER *ip, unsigned short *plength);

int net_ip_resolve(ETH_ADAPTER *adapter, IP_ENDPOINT *ep);
int net_ip_set_addr(ETH_ADAPTER *driver, IP_ADDR ip, IP_ADDR mask, IP_ADDR gateway);

unsigned short net_ip_checksum(NET16_T *data, unsigned byte_count);


#endif // NET_IP_H
