//  Simple DHCP Client
// by Luis Pons

#include "dhcp.h"
#include <kernel/thread.h>
#include <kernel/driver/net/udp.h>
#include <kernel/driver/net/udp_io.h>
#include <kernel/driver/net/adapter.h>
//#include "drivers.h"

#define NET_UDP_PORT_SERVER 67
#define NET_UDP_PORT_CLIENT 68

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

typedef enum
{
	DHCP_STATE_RESET = 0,
	DHCP_STATE_DISCOVER,
	DHCP_STATE_DISCOVER_WAIT,
	DHCP_STATE_REQUEST,
	DHCP_STATE_WAIT,
	DHCP_STATE_CONFIGURED,
} DHCP_STATE;

static UDP_IO_ENTRY _io;
static IP_ADDR _server_ip, _offered_ip;

static void dhcp_init_header(NET_DHCP_HEADER *dhcp, NET_ADAPTER *adapter, NET_DHCP_OPCODE opcode, unsigned long transaction)
{
	dhcp->Opcode = opcode;
	dhcp->HardwareType = NET_DHCP_HW_ETHERNET;
	dhcp->HardwareAddressLength = 6;
	dhcp->HopCount = 0;

	dhcp->TransactionId = transaction;
	dhcp->Seconds = HTON16(0);
	dhcp->Flags = HTON16(0);

	dhcp->ClientIP.Value = 0;
	dhcp->YourIP.Value = 0;
	dhcp->ServerIP.Value = 0;
	dhcp->GatewayIP.Value = 0;
	*((HW_ADDR *)dhcp->ClientHardwareAddress) = adapter->MAC;
	for(int i = 6; i < 16; i++) dhcp->ClientHardwareAddress[i] = 0;

	*dhcp->ServerHostName = '\0';
	*dhcp->BootFilename = '\0';
}

static unsigned long _transaction = 0;

static void dhcp_request ( NET_ADAPTER* driver)
{
	unsigned long _transaction = 0;
	// broadcast DHCP_REQUEST
	struct __attribute__((__packed__))
	{
		NET_DHCP_HEADER Header;
		NET32_T Magic;
		NET_DHCP_OP Op53;
		unsigned char Op53_Value;
		NET_DHCP_OP Op50;
		unsigned char Op50_Value[4];
		NET_DHCP_OP Op54;
		unsigned char Op54_Value[4];
		NET_DHCP_OP Op255;
	} op_request;

	dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, _transaction);
	op_request.Magic = NET_DHCP_MAGIC;
	
	// option 53
	op_request.Op53.Type = NET_DHCP_OPTION_MSG;
	op_request.Op53.Length = sizeof(op_request.Op53_Value);
	op_request.Op53_Value = NET_DHCP_MSG_REQUEST;

	// option 53, DHCP server
	op_request.Op54.Type = NET_DHCP_OPTION_SERVER_ID;
	op_request.Op54.Length = 4;
	*((IP_ADDR *)op_request.Op54_Value) = _server_ip;

	// option 50, Client IP
	op_request.Op50.Type = NET_DHCP_OPTION_REQUESTED_IP;
	op_request.Op50.Length = 4;
	*((IP_ADDR *)op_request.Op50_Value) = _offered_ip;

	// option 255, end
	op_request.Op255.Type = NET_DHCP_OPTION_END;
	op_request.Op255.Length = 0;

	int reply = 0;
	IP_PORT_ADDR remote;
	net_io_send((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);
}

static void dhcp_discover ( NET_ADAPTER* driver)
{
	// clear current ip value
	driver->IP.Value = 0;

	// broadcast DHCP_DISCOVER
	struct __attribute__((__packed__))
	{
		NET_DHCP_HEADER Header;
		NET32_T Magic;
		NET_DHCP_OP Op53;
		unsigned char Op53_Value;
	//			NET_DHCP_OP Op50;
	//			unsigned char Op50_Value[4];
		NET_DHCP_OP Op12;
		unsigned char Op12_Value[4];
		NET_DHCP_OP Op61;
		unsigned char Op61_Value[7];
		NET_DHCP_OP Op55;
		unsigned char Op55_Value[4];
		NET_DHCP_OP Op255;
	} op_request;

	dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, ++_transaction);
	op_request.Magic = NET_DHCP_MAGIC;

	// option 53
	op_request.Op53.Type = NET_DHCP_OPTION_MSG;
	op_request.Op53.Length = sizeof(op_request.Op53_Value);
	op_request.Op53_Value = NET_DHCP_MSG_DISCOVER;

	//		// option 50, Client IP
	//		op_request.Op50.Type = NET_DHCP_OPTION_REQUESTED_IP;
	//		op_request.Op50.Length = 4;
	//		*((IP_ADDR *)op_request.Op50_Value) = driver->IP;

	// option 12, Host Name
	op_request.Op12.Type = NET_DHCP_OPTION_HOSTNAME;
	op_request.Op12.Length = 4;
	op_request.Op12_Value[0] = 'K';
	op_request.Op12_Value[1] = 'I';
	op_request.Op12_Value[2] = 'O';
	op_request.Op12_Value[3] = '4';

	// option 61, Client Ident
	op_request.Op61.Type = NET_DHCP_OPTION_CLIENT_IDENT;
	op_request.Op61.Length = 7;
	op_request.Op61_Value[0] = 1; // ethernet
	*((HW_ADDR *)&op_request.Op61_Value[1]) = driver->MAC;

	// option 55, DHCP request
	op_request.Op55.Type = NET_DHCP_OPTION_REQUEST;
	op_request.Op55.Length = 4;
	op_request.Op55_Value[0] = 1;	// request subnet
	op_request.Op55_Value[1] = 3;	// request router
	op_request.Op55_Value[2] = 6;	// request dns
	op_request.Op55_Value[3] = 15; 

	// option 255, end
	op_request.Op255.Type = NET_DHCP_OPTION_END;
	op_request.Op255.Length = 0;

	NET_MBUF output_buffer;
	net_mbuf_init(&output_buffer,  &op_request, 0, sizeof(op_request));
	net_udp_send(driver, IP_ENDPOINT_BROADCAST, NET_UDP_PORT_CLIENT, NET_UDP_PORT_SERVER, &output_buffer);
}


static volatile DHCP_STATE _config_state = DHCP_STATE_RESET;

#define DHCP_THREAD_STACK (256)
static unsigned char _dhcp_thread_stack[ DHCP_THREAD_STACK];
static EXOS_THREAD _dhcp_thread;

static void* dhcp_thread_func ( void* arg)
{
	NET_ADAPTER* driver = (NET_ADAPTER*) arg;
	net_udp_io_create(&_io, EXOS_IOF_WAIT);
   	//exos_io_set_timeout((EXOS_IO_ENTRY *)&_io, 20); 

	IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = NET_UDP_PORT_CLIENT };
	int done = net_io_bind((NET_IO_ENTRY *)&_io, &local);

	_server_ip = driver->Gateway;
	_offered_ip = driver->IP;

	// Now that UDP is ready, start DHCP request
	dhcp_request ( driver);

	// Wait?
	dhcp_discover ( driver);
}

void dhcp_init (NET_ADAPTER *adapter)
{
	if ( adapter)
	{
		//exos_thread_set_pri(0);
		exos_thread_create(&_dhcp_thread, 1, _dhcp_thread_stack, DHCP_THREAD_STACK, NULL, dhcp_thread_func, adapter);
	}
}







#if 0

typedef enum
{
	DHCP_STATE_RESET = 0,
	DHCP_STATE_DISCOVER,
	DHCP_STATE_DISCOVER_WAIT,
	DHCP_STATE_REQUEST,
	DHCP_STATE_WAIT,
	DHCP_STATE_CONFIGURED,
} DHCP_STATE;

static volatile DHCP_STATE _config_state = DHCP_STATE_RESET;
static unsigned long _transaction = 0;
static int _timeout = 0;
static IP_ADDR _server_ip, _offered_ip;

static void _dhcp_handle(NET_DRIVER *driver, IP_HEADER *ip, UDP_HEADER *udp);
static void _dhcp_iterate(NET_DRIVER *driver, int elapsed);
static UDP_HANDLER _dhcp_handler = { NULL, NET_UDP_PORT_CLIENT, 0, &_dhcp_handle };

#define REQUEST_TIMEOUT 10000	// ms

void net_dhcp_iterate(int elapsed)
{
//	int drv_index = 0;
//	ETH_DRIVER *driver;
//	while(driver = net_drivers_iterate(&drv_index))
//	{
//		_dhcp_iterate(driver, elapsed);
//	}
}

static void _dhcp_iterate(NET_DRIVER *driver, int elapsed)
{
	if (_config_state == DHCP_STATE_RESET)
	{
		// install udp port handler
		int done = net_udp_add_port_handler(&_dhcp_handler);

		// prepare to request current ip
		_server_ip = driver->Gateway;
		_offered_ip = driver->IP;
		_config_state = DHCP_STATE_REQUEST;
	}
	else if (_config_state == DHCP_STATE_DISCOVER)
	{
		// clear current ip value
		driver->IP.Value = 0;

		// broadcast DHCP_DISCOVER
		struct __attribute__((__packed__))
		{
			NET_DHCP_HEADER Header;
			NET32_T Magic;
			NET_DHCP_OP Op53;
			unsigned char Op53_Value;
//			NET_DHCP_OP Op50;
//			unsigned char Op50_Value[4];
			NET_DHCP_OP Op12;
			unsigned char Op12_Value[4];
			NET_DHCP_OP Op61;
			unsigned char Op61_Value[7];
			NET_DHCP_OP Op55;
			unsigned char Op55_Value[4];
			NET_DHCP_OP Op255;
		} op_request;
		net_dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, ++_transaction);
		op_request.Magic = NET_DHCP_MAGIC;
		
		// option 53
		op_request.Op53.Type = NET_DHCP_OPTION_MSG;
		op_request.Op53.Length = sizeof(op_request.Op53_Value);
		op_request.Op53_Value = NET_DHCP_MSG_DISCOVER;

//		// option 50, Client IP
//		op_request.Op50.Type = NET_DHCP_OPTION_REQUESTED_IP;
//		op_request.Op50.Length = 4;
//		*((IP_ADDR *)op_request.Op50_Value) = driver->IP;

		// option 12, Host Name
		op_request.Op12.Type = NET_DHCP_OPTION_HOSTNAME;
		op_request.Op12.Length = 4;
		op_request.Op12_Value[0] = 'K';
		op_request.Op12_Value[1] = 'I';
		op_request.Op12_Value[2] = 'O';
		op_request.Op12_Value[3] = '4';

		// option 61, Client Ident
		op_request.Op61.Type = NET_DHCP_OPTION_CLIENT_IDENT;
		op_request.Op61.Length = 7;
		op_request.Op61_Value[0] = 1; // ethernet
		*((HW_ADDR *)&op_request.Op61_Value[1]) = driver->MAC;

		// option 55, DHCP request
		op_request.Op55.Type = NET_DHCP_OPTION_REQUEST;
		op_request.Op55.Length = 4;
		op_request.Op55_Value[0] = 1;	// request subnet
		op_request.Op55_Value[1] = 3;	// request router
		op_request.Op55_Value[2] = 6;	// request dns
		op_request.Op55_Value[3] = 15; 

		// option 255, end
		op_request.Op255.Type = NET_DHCP_OPTION_END;
		op_request.Op255.Length = 0;

		NET_MBUF output_buffer;
		net_mbuf_init(&output_buffer,  &op_request, 0, sizeof(op_request));
		net_udp_send(driver, IP_ENDPOINT_BROADCAST, NET_UDP_PORT_CLIENT, NET_UDP_PORT_SERVER, &output_buffer);
		_config_state = DHCP_STATE_DISCOVER_WAIT;
		_timeout = REQUEST_TIMEOUT;
	}
	else if (_config_state == DHCP_STATE_DISCOVER_WAIT)
	{
		if (_timeout > elapsed) _timeout -= elapsed;
		else _config_state = DHCP_STATE_DISCOVER;
	}
	else if (_config_state == DHCP_STATE_REQUEST)
	{
		// broadcast DHCP_REQUEST
		struct __attribute__((__packed__))
		{
			NET_DHCP_HEADER Header;
			NET32_T Magic;
			NET_DHCP_OP Op53;
			unsigned char Op53_Value;
			NET_DHCP_OP Op50;
			unsigned char Op50_Value[4];
			NET_DHCP_OP Op54;
			unsigned char Op54_Value[4];
			NET_DHCP_OP Op255;
		} op_request;
		net_dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, _transaction);
		op_request.Magic = NET_DHCP_MAGIC;
		
		// option 53
		op_request.Op53.Type = NET_DHCP_OPTION_MSG;
		op_request.Op53.Length = sizeof(op_request.Op53_Value);
		op_request.Op53_Value = NET_DHCP_MSG_REQUEST;

		// option 53, DHCP server
		op_request.Op54.Type = NET_DHCP_OPTION_SERVER_ID;
		op_request.Op54.Length = 4;
		*((IP_ADDR *)op_request.Op54_Value) = _server_ip;

		// option 50, Client IP
		op_request.Op50.Type = NET_DHCP_OPTION_REQUESTED_IP;
		op_request.Op50.Length = 4;
		*((IP_ADDR *)op_request.Op50_Value) = _offered_ip;

		// option 255, end
		op_request.Op255.Type = NET_DHCP_OPTION_END;
		op_request.Op255.Length = 0;

		NET_MBUF output_buffer;
		net_mbuf_init(&output_buffer,  &op_request, 0, sizeof(op_request));
		net_udp_send(driver, IP_ENDPOINT_BROADCAST, NET_UDP_PORT_CLIENT, NET_UDP_PORT_SERVER, &output_buffer);
		_config_state = DHCP_STATE_WAIT;
		_timeout = REQUEST_TIMEOUT;
	}
    else if (_config_state == DHCP_STATE_WAIT)
	{
		if (_timeout > elapsed) _timeout -= elapsed;
		else _config_state = DHCP_STATE_DISCOVER;
	}
}

void net_dhcp_init_header(NET_DHCP_HEADER *dhcp, NET_DRIVER *driver, NET_DHCP_OPCODE opcode, unsigned long transaction)
{
	dhcp->Opcode = opcode;
	dhcp->HardwareType = NET_DHCP_HW_ETHERNET;
	dhcp->HardwareAddressLength = 6;
	dhcp->HopCount = 0;

	dhcp->TransactionId = transaction;
	dhcp->Seconds = HTON16(0);
	dhcp->Flags = HTON16(0);

	dhcp->ClientIP.Value = 0;
	dhcp->YourIP.Value = 0;
	dhcp->ServerIP.Value = 0;
	dhcp->GatewayIP.Value = 0;
	*((HW_ADDR *)dhcp->ClientHardwareAddress) = driver->MAC;
	for(int i = 6; i < 16; i++) dhcp->ClientHardwareAddress[i] = 0;

	*dhcp->ServerHostName = '\0';
	*dhcp->BootFilename = '\0';
}

static int _parse_options(NET_DHCP_HEADER *dhcp, int options_length, int op_search, unsigned char *value, int value_storage_length)
{
	int result = 0;
	NET32_T magic = *((NET32_T *)dhcp->Options);
	if (magic.Value == NET_DHCP_MAGIC.Value)
	{
		int offset = 4; 	// skip magic
		while(offset < options_length)
		{
			NET_DHCP_OP *op = (NET_DHCP_OP *)&dhcp->Options[offset];
			if (op->Type == NET_DHCP_OPTION_PAD)	// pad byte
			{
				offset++;
				continue;
			}
			else if (op->Type == NET_DHCP_OPTION_END) // end tag
			{
				break;
			}
			else if (op->Type == op_search)
			{
				int length = op->Length;
				if (length > value_storage_length) length = value_storage_length;
				for (int i = 0; i < length; i++) value[i] = dhcp->Options[offset + 2 + i];
				offset += (op->Length + 2);
				result = 1;
				break;
			}
			else
			{
				offset += (op->Length + 2);
			}
		}
	}
	return result;
}

static void _dhcp_handle(NET_DRIVER *driver, IP_HEADER *ip, UDP_HEADER *udp)
{
	NET_DHCP_HEADER *dhcp = (NET_DHCP_HEADER *)udp->Data;
	int dhcp_length = NTOH16(udp->Length) - sizeof(UDP_HEADER);
	if (dhcp->Opcode == NET_DHCP_OPCODE_REPLY										// datagram is DHCP REPLY
		&& dhcp->TransactionId == _transaction										// pending transaction matches
		&& net_equal_hw_addr((HW_ADDR *)dhcp->ClientHardwareAddress, &driver->MAC))	// server sends to our MAC
	{
		int options_length = dhcp_length - sizeof(NET_DHCP_HEADER);
		unsigned char msg_type = 0;
		
		if (_parse_options(dhcp, options_length, NET_DHCP_OPTION_MSG, &msg_type, 1))
		{
			NET_DHCP_MSG_TYPE op_msg = (NET_DHCP_MSG_TYPE)msg_type;
			switch(op_msg)
			{
				case NET_DHCP_MSG_OFFER:
					if (_config_state == DHCP_STATE_DISCOVER_WAIT)
					{
						_server_ip = ip->SourceIP;
						_offered_ip = dhcp->YourIP;
						_config_state = DHCP_STATE_REQUEST;
					}
					break;
				case NET_DHCP_MSG_ACK:
					if (_config_state == DHCP_STATE_WAIT)
					{
						// change our ip config with server response!!
						driver->IP = dhcp->YourIP;
						IP_ADDR ip;
						if (_parse_options(dhcp, options_length, NET_DHCP_OPTION_NET_MASK, (void *)&ip, 4))
						{
							// acquire new net mask
							driver->NetMask = ip;
						}
						if (_parse_options(dhcp, options_length, NET_DHCP_OPTION_GATEWAY, (void *)&ip, 4))
						{
							// acquire new gateway
							driver->Gateway = ip;
						}

						_config_state = DHCP_STATE_CONFIGURED;
					}
					break;
				case NET_DHCP_MSG_NAK:
					_config_state = DHCP_STATE_DISCOVER;
					break;
			}
		}
	}
}

#endif



