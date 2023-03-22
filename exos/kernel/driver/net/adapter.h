#ifndef NET_ADAPTER_H
#define NET_ADAPTER_H

#include "net.h"
#include "support/phy.h"
#include "mbuf.h"
#include <kernel/dispatch.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <stdbool.h>

#define ETH_MAX_FRAME_SIZE 1536 // actually its 1512+crc(2) = 1514, but some tagging may require more
#define ETH_MAX_PAYLOAD 1500

typedef struct __net_driver net_driver_t;

typedef struct
{
	node_t Node;
	const char *Name;
	const net_driver_t *Driver;
	void *DriverData;
	dispatcher_context_t *Context;
	phy_t Phy;
	event_t InputEvent;
	mutex_t InputLock;
	mutex_t OutputLock;
	hw_addr_t MAC;
	unsigned short Speed;
	unsigned short MaxFrameSize;
	void *NetData;
} net_adapter_t;

typedef struct
{
	node_t Node;
	net_adapter_t *Adapter;
	net_mbuf_t Root;
} net_buffer_t;

typedef enum
{
	NET_ADAPTER_CTRL_LINK_UPDATE = 0,
	NET_ADAPTER_CTRL_LINK_RENEG,
} net_adapter_ctrl_t;

struct __net_driver
{
	bool (*Initialize)(net_adapter_t *adapter, unsigned phy_unit, const phy_handler_t *handler);
	void (*LinkUp)(net_adapter_t *adapter);
	void (*LinkDown)(net_adapter_t *adapter);
	bool (*PushInputBuffer)(net_adapter_t *adapter, net_buffer_t *buf);
	net_buffer_t *(*GetInputBuffer)(net_adapter_t *adapter);
	bool (*SendOutputBuffer)(net_adapter_t *adapter, net_buffer_t *buf);
	bool (*FreeBuffer)(net_adapter_t *adapter, net_buffer_t *buf);
	void (*Flush)(net_adapter_t *adapter);
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
void net_adapter_install(net_adapter_t *adapter, bool enable_network);
void net_adapter_list_lock();
void net_adapter_list_unlock();
bool net_adapter_enum(net_adapter_t **padapter);
//NET_ADAPTER *net_adapter_find(IP_ADDR addr);
//NET_ADAPTER *net_adapter_find_gateway(IP_ADDR addr);

bool net_adapter_control(net_adapter_t *adapter, net_adapter_ctrl_t ctrl);

net_buffer_t *net_adapter_alloc_buffer(net_adapter_t *adapter);
void net_adapter_free_buffer(net_buffer_t *buf);
void net_adapter_flush(net_adapter_t *adapter);

net_buffer_t *net_adapter_get_input(net_adapter_t *adapter);
bool net_adapter_send_output(net_adapter_t *adapter, net_buffer_t *buf);
bool net_adapter_push_input(net_adapter_t *adapter, net_buffer_t *buf);

#endif // NET_DRIVERS_H

