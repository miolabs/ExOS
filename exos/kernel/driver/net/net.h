#ifndef NET_NET_H
#define NET_NET_H

#include <stdbool.h>

typedef union __attribute__((__packed__))
{
	unsigned char Bytes[4];
	unsigned long Value;
} net32_t;

typedef struct __attribute__((__packed__))
{
	unsigned char MSB;
	unsigned char LSB;
} net16_t;

#define NTOH16(s) (((s).LSB)|((s).MSB<<8))
#define HTON16(v) (net16_t){(unsigned char)((v)>>8), (unsigned char)(v)} 
#define NTOH32(s) (((s).Bytes[3])|((s).Bytes[2]<<8)|((s).Bytes[1]<<16)|((s).Bytes[0]<<24))
#define HTON32(v) (net32_t){(unsigned char)((v)>>24), (unsigned char)((v)>>16), (unsigned char)((v)>>8), (unsigned char)(v)} 

typedef enum
{
	ETH_TYPE_ARP = 0x0806,
	ETH_TYPE_IP = 0x0800,
	ETH_TYPE_IPv6 = 0x86dd,
	ETH_TYPE_ETHERCAT = 0x88a4,
} eth_type_t;

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[6];
} hw_addr_t;

typedef struct __attribute__((__packed__))
{
	hw_addr_t Destination;
	hw_addr_t Sender;
	net16_t Type;
} eth_header_t;


#ifdef EXOS_OLD
typedef hw_addr_t HW_ADDR;
typedef net32_t NET32_T;
typedef net16_t NET16_T;
#endif

// prototypes
bool net_equal_hw_addr(hw_addr_t *a, hw_addr_t *b);
bool net_hw_addr_parse(hw_addr_t *addr, const char *mac);

#endif // NET_NET_H
