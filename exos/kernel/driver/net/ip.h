#ifndef NET_IP_H
#define NET_IP_H

#include "net_service.h"
#include "adapter.h"

typedef union __attribute__((__packed__))
{
	unsigned char Bytes[4];
	unsigned long Value;
} ip_addr_t;

typedef struct
{
	net_config_t Base;
	ip_addr_t IP;
	ip_addr_t Mask;
	// TODO
} net_ip_config_t;

typedef enum
{
	IP_VER_RESERVED = 0,
	IP_VER_IPv4 = 4,
	IP_VER_ST = 5,
	IP_VER_IPv6 = 6,
} ip_version_t;

typedef enum
{
	IP_PROTOCOL_ICMP = 1,
	IP_PROTOCOL_ST = 5,
	IP_PROTOCOL_TCP = 6,
	IP_PROTOCOL_UDP = 17,
	IP_PROTOCOL_UDP_LITE = 136,
} ip_protocol_t;

typedef struct __attribute__((__packed__))
{
	struct __attribute__((__packed__))
	{
		unsigned HeaderLength:4;
		ip_version_t Version:4;
	};
	unsigned char DiffServ;
	net16_t TotalLength;
	net16_t Id;
	net16_t Fragment;
	unsigned char TTL;
	ip_protocol_t Protocol:8;
	net16_t HeaderChecksum;
	ip_addr_t SourceIP;
	ip_addr_t DestinationIP;
} ip_header_t;

#define IP_FRAGF_RF		0x8000	// Reserved Fragment
#define IP_FRAGF_DF		0x4000	// Don't Fragment
#define IP_FRAGF_MF		0x2000	// More Fragments
#define IP_FRAGF_OFFSET	0x1fff

typedef struct
{
	ip_addr_t SourceIP;
	ip_addr_t DestinationIP;
	unsigned char Reserved;
	unsigned char Protocol;
	net16_t TotalLength;
} IP_PSEUDO_HEADER;

typedef struct __attribute__((__packed__))
{
	hw_addr_t MAC;
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
	ip_addr_t IP;
} IP_ENDPOINT;

#define DSCP_CS1	0b001000	// background
#define DSCP_AF11	0b001010	// video
#define DSCP_CS5	0b101000	// video
#define DSCP_AF12	0b001100	// voice
#define DSCP_CS7	0b111000	// voice
#define DSCP_AF41	0b100010	// voice
#define DSCP_EF		0b101110	// voice

extern const IP_ENDPOINT __ep_broadcast;

#define IP_ENDPOINT_BROADCAST ((IP_ENDPOINT *)&__ep_broadcast)
#define IP_ADDR_BROADCAST (ip_addr_t){255, 255, 255, 255}
#define IP_ADDR_ANY (ip_addr_t){0, 0, 0, 0}

typedef struct
{
	ip_addr_t Address;
	unsigned short Port;
} ip_port_addr_t;

#ifdef EXOS_OLD
typedef ip_addr_t IP_ADDR;
#endif

// prototypes
void net_ip_initialize();
void net_ip_init_config(net_adapter_t *adapter);
bool net_ip_get_config(net_adapter_t *adapter, net_ip_config_t **pconfig); 
bool net_ip_input(net_adapter_t *adapter, eth_header_t *eth, ip_header_t *ip);
void *net_ip_output(net_adapter_t *adapter, net_buffer_t *output, unsigned hdr_size, const IP_ENDPOINT *destination, ip_protocol_t protocol);
int net_ip_send_output(net_adapter_t *adapter, net_buffer_t *output, unsigned payload);
void *net_ip_get_payload(ip_header_t *ip, unsigned short *plength);

int net_ip_get_adapter_and_resolve(net_adapter_t **padapter, IP_ENDPOINT *ep);
int net_ip_resolve(net_adapter_t *adapter, IP_ENDPOINT *ep);
int net_ip_set_addr(net_adapter_t *driver, ip_addr_t ip, ip_addr_t mask, ip_addr_t gateway);

unsigned short net_ip_checksum(net16_t *data, unsigned byte_count);


#endif // NET_IP_H
