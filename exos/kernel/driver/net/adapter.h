#ifndef NET_ADAPTER_H
#define NET_ADAPTER_H

#include "net.h"
#include "support/phy.h"
#include "mbuf.h"
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <stdbool.h>

//#ifndef NET_ADAPTER_THREAD_STACK
//#define NET_ADAPTER_THREAD_STACK 768
//#endif

typedef struct __attribute__((__packed__))
{
	hw_addr_t Destination;
	hw_addr_t Sender;
	net16_t Type;
} eth_header_t;

typedef enum
{
	ETH_TYPE_ARP = 0x0806,
	ETH_TYPE_IP = 0x0800,
	ETH_TYPE_IPv6 = 0x86dd,
	ETH_TYPE_ETHERCAT = 0x88a4,
} eth_type_t;

#define ETH_MAX_FRAME_SIZE 1536
#define ETH_MAX_PAYLOAD 1500

typedef struct __net_driver net_driver_t;

typedef struct
{
	node_t Node;
	const net_driver_t *Driver;
	phy_t Phy;
	mutex_t InputLock;
	mutex_t OutputLock;
	hw_addr_t MAC;
	unsigned short Speed;
	//ip_addr_t IP;
	//ip_addr_t NetMask;
	//ip_addr_t Gateway;

	//unsigned long InputSignal;
	//exos_thread_t Thread;
	//unsigned char Stack[NET_ADAPTER_THREAD_STACK];
} net_adapter_t;

typedef struct
{
	node_t Node;
	net_adapter_t *Adapter;
	void *Buffer;
	unsigned short Offset;
	unsigned short Length;
} net_buffer_t;

typedef struct
{
	event_t *CompletedEvent;
	net_mbuf_t Buffer;
} NET_OUTPUT_BUFFER;

typedef void(* NET_CALLBACK)(void *state);

struct __net_driver
{
	bool (*Initialize)(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
	void (*LinkUp)(net_adapter_t *adapter);
	void (*LinkDown)(net_adapter_t *adapter);
	void *(*GetInputBuffer)(net_adapter_t *adapter, unsigned long *plength);
	void (*DiscardInputBuffer)(net_adapter_t *adapter, void *buffer);
	void *(*GetOutputBuffer)(net_adapter_t *adapter, unsigned long size);
	int (*SendOutputBuffer)(net_adapter_t *adapter, net_mbuf_t *mbuf, NET_CALLBACK callback, void *state);
};

#ifdef EXOS_OLD
typedef eth_header_t ETH_HEADER;
typedef eth_type_t ETH_TYPE;
typedef net_adapter_t NET_ADAPTER;
typedef net_buffer_t NET_BUFFER;
#endif

// prototypes
void net_adapter_initialize();
bool net_adapter_create(net_adapter_t *adapter, const net_driver_t *driver, unsigned unit, const phy_handler_t *handler);
void net_adapter_install(net_adapter_t *adapter);
void net_adapter_list_lock();
void net_adapter_list_unlock();
bool net_adapter_enum(net_adapter_t **padapter);
//NET_ADAPTER *net_adapter_find(IP_ADDR addr);
//NET_ADAPTER *net_adapter_find_gateway(IP_ADDR addr);

void net_adapter_input(net_adapter_t *adapter);

void *net_adapter_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *buf, unsigned hdr_size, const hw_addr_t *destination, eth_type_t type);
int net_adapter_send_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *buf);

net_buffer_t *net_adapter_alloc_buffer(net_adapter_t *adapter, void *buffer, void *data, unsigned long length);
void net_adapter_discard_input_buffer(net_buffer_t *packet);


#endif // NET_DRIVERS_H

