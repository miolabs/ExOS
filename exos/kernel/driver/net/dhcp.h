#ifndef NET_DHCP_H
#define NET_DHCP_H

#include <net/adapter.h>

typedef struct __attribute__((__packed__))
{
	unsigned char Opcode;
	unsigned char HardwareType;
	unsigned char HardwareAddressLength;
	unsigned char HopCount;
	unsigned long TransactionId;
	NET16_T Seconds;
	NET16_T Flags;
	IP_ADDR ClientIP;
	IP_ADDR YourIP;
	IP_ADDR ServerIP;
	IP_ADDR GatewayIP;
	unsigned char ClientHardwareAddress[16];
	unsigned char ServerHostName[64];
	unsigned char BootFilename[128];
	unsigned char Options[0];
} NET_DHCP_HEADER;

typedef enum
{
	NET_DHCP_OPCODE_REQUEST = 1,
	NET_DHCP_OPCODE_REPLY = 2,
} NET_DHCP_OPCODE;

typedef enum
{
	NET_DHCP_HW_ETHERNET = 1,
} NET_DHCP_HW_TYPE;

#define NET_DHCP_MAGIC (NET32_T){99, 130, 83, 99}

typedef enum
{
	NET_DHCP_MSG_DISCOVER = 1,
    NET_DHCP_MSG_OFFER = 2,
    NET_DHCP_MSG_REQUEST = 3,
    NET_DHCP_MSG_DECLINE = 4,
	NET_DHCP_MSG_ACK = 5,
    NET_DHCP_MSG_NAK = 6,
    NET_DHCP_MSG_RELEASE = 7,
	NET_DHCP_MSG_INFORM = 8,
} NET_DHCP_MSG_TYPE;

typedef struct __attribute__((__packed__))
{
	unsigned char Type;
	unsigned char Length;
} NET_DHCP_OP; 

typedef enum 
{
	NET_DHCP_OPTION_PAD = 0,
	NET_DHCP_OPTION_NET_MASK = 1,
	NET_DHCP_OPTION_GATEWAY = 3,
	NET_DHCP_OPTION_HOSTNAME = 12,
	NET_DHCP_OPTION_REQUESTED_IP = 50,
	NET_DHCP_OPTION_MSG = 53,
	NET_DHCP_OPTION_SERVER_ID = 54,
	NET_DHCP_OPTION_REQUEST = 55,
	NET_DHCP_OPTION_CLIENT_IDENT = 61,
	NET_DHCP_OPTION_END = 255,
} NET_DHCP_OPTION;

#define NET_UDP_PORT_SERVER 67
#define NET_UDP_PORT_CLIENT 68

// prototypes
void net_dhcp_init_header(NET_DHCP_HEADER *dhcp, NET_ADAPTER *adapter, NET_DHCP_OPCODE opcode, unsigned long transaction);
void net_dhcp_iterate(int elapsed);

#endif // NET_DHCP_H
