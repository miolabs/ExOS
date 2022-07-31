//  Simple DHCP Client
// by Luis Pons

//#include "dns.h"

#include "dhcp.h"
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <kernel/io.h>
#include <kernel/driver/net/udp.h>
#include <kernel/driver/net/udp_io.h>
#include <kernel/driver/net/net_io.h>
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
	NET_DHCP_OPTION_VENDOR_IDENT = 60,
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
	DHCP_STATE_INFORM,
} DHCP_STATE;

static unsigned long _transaction = 0x85742698;
static UDP_IO_ENTRY _io;


static IP_ADDR _server_ip, _offered_ip;
static volatile DHCP_STATE _config_state = DHCP_STATE_RESET;
static int _timeout = 1;
unsigned long _delay = 0;
unsigned long _time = 0;

#define DHCP_THREAD_STACK (1024)
#define REQUEST_TIMEOUT 10000	// ms
#define BROADCAST_FLAG HTON16(0x8000)

static unsigned char _dhcp_thread_stack[ DHCP_THREAD_STACK];
static EXOS_THREAD _dhcp_thread;

//dchp header

static void dhcp_init_header(NET_DHCP_HEADER *dhcp, NET_ADAPTER *adapter, NET_DHCP_OPCODE opcode, unsigned long transaction);


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
	for(int i = 0; i < 64; i++) dhcp->ServerHostName[i] = 0;
	for(int i = 0; i < 128; i++) dhcp->BootFilename[i] = 0;
}


static void dhcp_request ( NET_ADAPTER* driver)
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

		dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, _transaction);
		op_request.Header.Flags = BROADCAST_FLAG;
		op_request.Magic = NET_DHCP_MAGIC;
	
		// option 53
		op_request.Op53.Type = NET_DHCP_OPTION_MSG;
		op_request.Op53.Length = sizeof(op_request.Op53_Value);
		op_request.Op53_Value = NET_DHCP_MSG_REQUEST;

		// option 54, DHCP server
		op_request.Op54.Type = NET_DHCP_OPTION_SERVER_ID;
		op_request.Op54.Length = 4;
		*((IP_ADDR *)op_request.Op54_Value) = _server_ip;
	
		// option 50, Client IP
		op_request.Op50.Type = NET_DHCP_OPTION_REQUESTED_IP;
		op_request.Op50.Length = 4;
		*((IP_ADDR *)op_request.Op50_Value) = _offered_ip;

		// option 255, end
		op_request.Op255.Type = NET_DHCP_OPTION_END;
		op_request.Op255.Length = 1;

		int reply = sizeof(op_request);
		IP_PORT_ADDR remote = { .Address = {255, 255, 255, 255}, .Port = NET_UDP_PORT_SERVER };
		int done = 0;
		if(_delay == 0)
		{
			_time = exos_timer_time();
		}
		while (done == 0)
		{
			exos_thread_sleep(_timeout*1000);
			_delay = exos_timer_elapsed(_time);

			if(_timeout != 1){op_request.Header.Seconds = (NET16_T){(unsigned char)(_timeout)};}
			done = net_io_send((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);
		}
}


static void dhcp_discover ( NET_ADAPTER* driver)
{
	// clear current ip value
	driver->IP.Value = 0;
	_transaction = _transaction+27;
	// broadcast DHCP_DISCOVER
	struct __attribute__((__packed__))
	{
		NET_DHCP_HEADER Header;
		NET32_T Magic;
		NET_DHCP_OP Op53;
		unsigned char Op53_Value;
    	NET_DHCP_OP Op61;
		unsigned char Op61_Value[7];
		NET_DHCP_OP Op12;
		unsigned char Op12_Value[8];
		NET_DHCP_OP Op55;
		unsigned char Op55_Value[4];
		NET_DHCP_OP Op255;
	} op_request;

	dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, _transaction);
    op_request.Header.Flags = BROADCAST_FLAG;
	op_request.Magic = NET_DHCP_MAGIC;

	// option 53
	op_request.Op53.Type = NET_DHCP_OPTION_MSG;
	op_request.Op53.Length = sizeof(op_request.Op53_Value);
	op_request.Op53_Value = NET_DHCP_MSG_DISCOVER;

	// option 12, Host Name
	op_request.Op12.Type = NET_DHCP_OPTION_HOSTNAME;
	op_request.Op12.Length = 8;
	op_request.Op12_Value[0] = 'S';
	op_request.Op12_Value[1] = 'T';
	op_request.Op12_Value[2] = 'A';
	op_request.Op12_Value[3] = 'R';
	op_request.Op12_Value[4] = 'R';
	op_request.Op12_Value[5] = 'R';
	op_request.Op12_Value[6] = 'R';
	op_request.Op12_Value[7] = 'a';

	// option 61, Client Ident
	op_request.Op61.Type = NET_DHCP_OPTION_CLIENT_IDENT;
	op_request.Op61.Length = 7;
	op_request.Op61_Value[0] = 1; // ethernet
	*((HW_ADDR *)&op_request.Op61_Value[1]) = driver->MAC;

	// option 55, DHCP request
	op_request.Op55.Type = NET_DHCP_OPTION_REQUEST;
	op_request.Op55.Length = 4;//12
	op_request.Op55_Value[0] = 1;	// request subnet
	op_request.Op55_Value[1] = 15;	// request router
	op_request.Op55_Value[2] = 3;	// request dns
	op_request.Op55_Value[3] = 6; 

	// option 255, end
	op_request.Op255.Type = NET_DHCP_OPTION_END;
	op_request.Op255.Length = 0;

	int reply = sizeof(op_request);
	IP_PORT_ADDR remote = { .Address = {255, 255, 255, 255}, .Port = NET_UDP_PORT_SERVER };
	int done = 0;
	while (done == 0)
	{
		done = net_io_send((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);
	}
}


//static void dhcp_inform ( NET_ADAPTER* driver)
//{
//	driver->IP = (IP_ADDR) {192, 168, 1, 130};
//	_transaction = _transaction+7;
//	// broadcast DHCP_DISCOVER
//	struct __attribute__((__packed__))
//	{
//		NET_DHCP_HEADER Header;
//		NET32_T Magic;
//		NET_DHCP_OP Op53;
//		unsigned char Op53_Value;
//    	NET_DHCP_OP Op61;
//		unsigned char Op61_Value[7];
//		NET_DHCP_OP Op12;
//		unsigned char Op12_Value[8];
//		NET_DHCP_OP Op55;
//		unsigned char Op55_Value[4];
//		NET_DHCP_OP Op255;
//	} op_request;
//
//	dhcp_init_header(&op_request.Header, driver, NET_DHCP_OPCODE_REQUEST, _transaction);
//    op_request.Header.Flags = BROADCAST_FLAG;
//	op_request.Header.ClientIP = driver->IP;
//	op_request.Magic = NET_DHCP_MAGIC;
//
//	// option 53
//	op_request.Op53.Type = NET_DHCP_OPTION_MSG;
//	op_request.Op53.Length = sizeof(op_request.Op53_Value);
//	op_request.Op53_Value = NET_DHCP_MSG_INFORM;
//
//	// option 12, Host Name
//	op_request.Op12.Type = NET_DHCP_OPTION_HOSTNAME;
//	op_request.Op12.Length = 8;
//	op_request.Op12_Value[0] = 'S';
//	op_request.Op12_Value[1] = 'T';
//	op_request.Op12_Value[2] = 'A';
//	op_request.Op12_Value[3] = 'R';
//	op_request.Op12_Value[4] = 'R';
//	op_request.Op12_Value[5] = 'R';
//	op_request.Op12_Value[6] = 'R';
//	op_request.Op12_Value[7] = 'a';
//
//	// option 61, Client Ident
//	op_request.Op61.Type = NET_DHCP_OPTION_CLIENT_IDENT;
//	op_request.Op61.Length = 7;
//	op_request.Op61_Value[0] = 1; // ethernet
//	*((HW_ADDR *)&op_request.Op61_Value[1]) = driver->MAC;
//
//	// option 55, DHCP request
//	op_request.Op55.Type = NET_DHCP_OPTION_REQUEST;
//	op_request.Op55.Length = 4;//12
//	op_request.Op55_Value[0] = 1;	// request subnet
//	op_request.Op55_Value[1] = 15;	// request router
//	op_request.Op55_Value[2] = 3;	// request dns
//	op_request.Op55_Value[3] = 6; 
//
//	// option 255, end
//	op_request.Op255.Type = NET_DHCP_OPTION_END;
//	op_request.Op255.Length = 0;
//
//	int reply = sizeof(op_request);
//	IP_PORT_ADDR remote = { .Address = {255, 255, 255, 255}, .Port = NET_UDP_PORT_SERVER };
//	int done = 0;
//	while (done == 0)
//	{
//		done = net_io_send((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);
//	}
//}


static void dhcp_offer ( NET_ADAPTER* driver)
{	
	// broadcast DHCP_OFFER
	struct __attribute__((__packed__))
	{
		NET_DHCP_HEADER Header;
        NET32_T Magic;
		NET_DHCP_OP Op53;
		unsigned char Op53_Value[50];
	} op_request;

	int reply = sizeof(op_request);
	IP_PORT_ADDR remote;
	exos_io_set_timeout((EXOS_IO_ENTRY *)&_io, 5000);
	int done = 0;
	while (done == 0)
	{
		done = net_io_receive((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);

		if (op_request.Op53_Value[0] == NET_DHCP_MSG_OFFER && op_request.Header.TransactionId == _transaction)
		{
			_offered_ip = (IP_ADDR) op_request.Header.YourIP;

			//Bucle to looking for the value of router option
			unsigned char *dir;
			dir = &op_request.Op53.Type;
			int length = 0; // Option length

            while(*dir != 0xff)
			{
				length = (*(dir+1));
				
				if(*(dir) == NET_DHCP_OPTION_SERVER_ID)
				{
					_server_ip = (IP_ADDR) { *(dir+2), *(dir+3), *(dir+4), *(dir+5)};
				}
				dir = dir + 2 + length; // Next Option
			}

			_config_state = DHCP_STATE_REQUEST;
		}
		else
		{
			_config_state = DHCP_STATE_DISCOVER;
			_transaction = _transaction + 84;
		}

	}
}


static void dhcp_read_request ( NET_ADAPTER* driver)
{	
	// broadcast DHCP_ACK_REQUEST
	struct __attribute__((__packed__))
	{
		NET_DHCP_HEADER Header;
        NET32_T Magic;
		NET_DHCP_OP Op53;
		unsigned char Op53_Value[50];
	} op_request;

	int reply = sizeof(op_request);
	IP_PORT_ADDR remote;
	exos_io_set_timeout((EXOS_IO_ENTRY *)&_io, 1000);
	int done = 0;
	while (done == 0)
	{
		done = net_io_receive((NET_IO_ENTRY *)&_io, &op_request, reply, &remote);

		if (op_request.Header.TransactionId == _transaction && op_request.Op53_Value[0] == NET_DHCP_MSG_ACK)
		{
			driver->IP = (IP_ADDR) op_request.Header.YourIP;
			
			//Bucle to looking for the value of net mask option and gateway option
			unsigned char *dir;
			dir = &op_request.Op53.Type;
			int length = 0; // Option length

			while(*dir != 0xff)
			{
				length = (*(dir+1));
				
				if(*(dir) == NET_DHCP_OPTION_NET_MASK)
				{
					driver->NetMask =(IP_ADDR) { *(dir+2), *(dir+3), *(dir+4), *(dir+5)};
				}
				else if(*(dir) == NET_DHCP_OPTION_GATEWAY)
				{
					driver->Gateway = (IP_ADDR) { *(dir+2), *(dir+3), *(dir+4), *(dir+5)};
				}
				
				dir = dir + 2 + length; // Next Option
			}
						
			_config_state = DHCP_STATE_CONFIGURED;
		}
		else if(op_request.Header.TransactionId == _transaction && op_request.Op53_Value[0] == NET_DHCP_MSG_NAK)
		{
			_config_state = DHCP_STATE_DISCOVER; //Define the state
			_timeout = 1;
			_time = 0;
			_delay = 0;
		}
		else
		{
			//increment timeout
			_config_state = DHCP_STATE_REQUEST; //Define the state 
			_timeout = _timeout + 2*_timeout;

			if(_timeout > 14)
			{
				_timeout = 1;
				_transaction = _transaction + 52;
                _config_state = DHCP_STATE_DISCOVER;
			}
		}
	}
}


static void* dhcp_thread_func ( void* arg)
{
	NET_ADAPTER* driver = (NET_ADAPTER*) arg;
	// Now that UDP is ready, start DHCP request
	//exos_thread_sleep(1000);

	//States Machine
	while (1)
	{
		if(_config_state == DHCP_STATE_RESET)
		{
			net_udp_io_create(&_io, EXOS_IOF_WAIT);

			IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = NET_UDP_PORT_CLIENT };
			int done = net_io_bind((NET_IO_ENTRY *)&_io, &local);  //done = 0 --> ok!
			if (done == 0)
			{			
				_server_ip = driver->Gateway;
				_offered_ip = driver->IP;
				_config_state = DHCP_STATE_REQUEST; //DHCP_STATE_DISCOVER;
			}
			else{_config_state = DHCP_STATE_RESET;}
		}
//		else if (_config_state == DHCP_STATE_INFORM) //TEST INFORM
//		{
//			dhcp_inform (driver);
//            _config_state = DHCP_STATE_WAIT;
//		}
		else if (_config_state == DHCP_STATE_CONFIGURED)
		{
			//Ip address assigned
			exos_thread_sleep(1000);
			if(driver->Speed == 0)
			{
				_config_state = DHCP_STATE_RESET;
			}
		}
		else if (_config_state == DHCP_STATE_DISCOVER)
		{
			dhcp_discover (driver);
			_config_state = DHCP_STATE_DISCOVER_WAIT;
		}
		else if (_config_state == DHCP_STATE_DISCOVER_WAIT)
		{
			dhcp_offer (driver);
		}
		else if (_config_state == DHCP_STATE_WAIT)
		{
			dhcp_read_request(driver);
		}
		else if (_config_state == DHCP_STATE_REQUEST)
		{
			dhcp_request ( driver);
			_config_state = DHCP_STATE_WAIT;
		}
	}

}

void dhcp_init (NET_ADAPTER *adapter)
{
	if ( adapter)
	{
		exos_thread_create(&_dhcp_thread, 1, _dhcp_thread_stack, DHCP_THREAD_STACK, NULL, dhcp_thread_func, adapter);
	}
}




