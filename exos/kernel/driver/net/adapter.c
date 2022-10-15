#include "adapter.h"
#include "board.h"
#include "mbuf.h"
#include "arp.h"
#include "arp_tables.h"
#include "ip.h"
#include "net_service.h"
#include <kernel/fifo.h>
#include <kernel/panic.h>

#ifdef ENABLE_ETHERCAT
#include <support/ethercat/ethercat.h>
#endif

static mutex_t _adapters_lock;
static list_t _adapters;
static fifo_t _free_wrappers;
static event_t _free_wrapper_event;

#define NET_PACKET_WRAPPERS 16
static net_buffer_t _wrappers[NET_PACKET_WRAPPERS];

void net_adapter_initialize()
{
	list_initialize(&_adapters);
	exos_event_create(&_free_wrapper_event, EXOS_EVENTF_AUTORESET);
	exos_fifo_create(&_free_wrappers, &_free_wrapper_event);
	for(int i = 0; i < NET_PACKET_WRAPPERS; i++)
	{
		_wrappers[i] = (net_buffer_t) { .Adapter = NULL }; 
		exos_fifo_queue(&_free_wrappers, (node_t *)&_wrappers[i]);
	}

	exos_mutex_create(&_adapters_lock);
	net_adapter_t *adapter;
	const phy_handler_t *handler;
	int index = 0;
	while(NULL != (adapter = net_board_get_adapter(index)))
	{
		// FIXME: board should provide phy addr (unit) and phy handler (driver)
		if (net_adapter_create(adapter, adapter->Driver, 0, NULL))
		{
			net_board_set_mac_address(adapter, index);

			net_adapter_install(adapter);
			
			net_board_set_ip_address(adapter, index);
		}

		index++;
	}
}

bool net_adapter_create(net_adapter_t *adapter, const net_driver_t *driver, unsigned phy_unit, const phy_handler_t *handler)
{
	ASSERT(adapter != NULL && driver != NULL, KERNEL_ERROR_NULL_POINTER);
	*adapter = (net_adapter_t) { .Driver = driver, 
		.Node = (node_t) { .Type = EXOS_NODE_IO_DEVICE } };

	exos_mutex_create(&adapter->InputLock);
	exos_mutex_create(&adapter->OutputLock);

	exos_event_create(&adapter->InputEvent, EXOS_EVENTF_AUTORESET);
	return driver->Initialize(adapter, phy_unit, handler);
}

void net_adapter_install(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL && adapter->Driver != NULL, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&_adapters_lock);

	ASSERT(!list_find_node(&_adapters, &adapter->Node), KERNEL_ERROR_KERNEL_PANIC);
	list_add_tail(&_adapters, &adapter->Node);

	net_service_start(adapter);
	exos_mutex_unlock(&_adapters_lock);
}


void net_adapter_list_lock()
{
	exos_mutex_lock(&_adapters_lock);
}

void net_adapter_list_unlock()
{
	exos_mutex_unlock(&_adapters_lock);
}

int net_adapter_enum(net_adapter_t **padapter)
{
	net_adapter_t *adapter = *padapter;
#ifdef DEBUG
	if (adapter != NULL &&
		NULL == list_find_node(&_adapters, &adapter->Node))
	{
		*padapter = NULL;
		return 0;
	}
#endif

	adapter = (adapter == NULL) ? (!LIST_ISEMPTY(&_adapters) ? (net_adapter_t *)(LIST_HEAD(&_adapters)->Succ) : NULL) : 
		(net_adapter_t *)NODE_SUCC(&_adapters, &adapter->Node);
	*padapter = adapter;
	return (adapter != NULL);
}

#if 0
net_adapter_t *net_adapter_find(ip_addr_t addr)
{
	net_adapter_t *found = NULL;
	net_adapter_list_lock();
	FOREACH(node, &_adapters)
	{
		net_adapter_t *adapter = (net_adapter_t *)node;
		ASSERT(adapter->Node.Type == EXOS_NODE_IO_DEVICE, KERNEL_ERROR_WRONG_NODE);

		ip_addr_t network = (ip_addr_t) { .Value = (adapter->IP.Value & adapter->NetMask.Value) };
		if (network.Value == (addr.Value & adapter->NetMask.Value))
		{
			found = adapter;
			break;
		}
	}
    net_adapter_list_unlock();
	return found;
}

net_adapter_t *net_adapter_find_gateway(ip_addr_t addr)
{
	net_adapter_t *found = NULL;
	net_adapter_list_lock();
	FOREACH(node, &_adapters)
	{
		net_adapter_t *adapter = (net_adapter_t *)node;
		ASSERT(adapter->Node.Type == EXOS_NODE_IO_DEVICE, KERNEL_ERROR_WRONG_NODE);

		if (adapter->Gateway.Value != 0)
		{
			found = adapter;
			break;
		}
	}
    net_adapter_list_unlock();
	return found;
}
#endif

bool net_adapter_control(net_adapter_t *adapter, net_adapter_ctrl_t ctrl)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	bool done = false;
	phy_t *phy = &adapter->Phy;
	switch(ctrl)
	{
		case NET_ADAPTER_CTRL_LINK_UPDATE:
			done = phy_read_link_state(phy);
			break;
	}
	return done;
}

bool net_adapter_get_input(net_adapter_t *adapter, net_buffer_t *buf)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
   	const net_driver_t *driver = adapter->Driver;
	
	unsigned length;
	eth_header_t *frame = driver->GetInputBuffer(adapter, &length);
	if (frame != NULL)
	{
		buf->Adapter = adapter;
		buf->Buffer =  frame;
		buf->Length = length;
		return true;
	}
	return false;
}

void net_adapter_input(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
   	const net_driver_t *driver = adapter->Driver;

	while(1)
	{
		unsigned length;
		eth_header_t *buffer = driver->GetInputBuffer(adapter, &length);
		if (buffer == NULL) break;

		eth_type_t packet_type = NTOH16(buffer->Type);
		void *payload = (void *)buffer + sizeof(eth_header_t);

		int queued = 0;
		switch(packet_type)
		{
			case ETH_TYPE_ARP:
				net_arp_input(adapter, (ARP_HEADER *)payload);
				break;
			case ETH_TYPE_IP:
				queued = net_ip_input(adapter, buffer, (IP_HEADER *)payload);
				break;
#ifdef ENABLE_ETHERCAT
			case ETH_TYPE_ETHERCAT:
				queued = net_ecat_input(adapter, buffer, (ECAT_HEADER *)payload);
				break;
#endif
		}
		if (!queued) 
			driver->DiscardInputBuffer(adapter, buffer);
	}
}

void *net_adapter_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *output, unsigned hdr_size, const hw_addr_t *destination, eth_type_t type)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	const net_driver_t *driver = adapter->Driver;

	hdr_size += sizeof(eth_header_t);
	eth_header_t *buffer = driver->GetOutputBuffer(adapter, hdr_size);
	if (buffer != NULL)
	{
		buffer->Sender = adapter->MAC;
		buffer->Destination = *destination;
		buffer->Type = HTON16(type);
		
		// initialize mbuf
		net_mbuf_init(&output->Buffer, buffer, 0, hdr_size);

		return (void *)buffer + sizeof(eth_header_t);
	}
	return NULL;
}

static void _send_callback(void *state)
{
	event_t *event = (event_t *)state;
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
	exos_event_set(event);
}

int net_adapter_send_output(net_adapter_t *adapter, NET_OUTPUT_BUFFER *output)
{
	const net_driver_t *driver = adapter->Driver;
	int done = driver->SendOutputBuffer(adapter, &output->Buffer, 
		output->CompletedEvent != NULL ? _send_callback : NULL, output->CompletedEvent);
	return done;
}

//net_buffer_t *net_adapter_alloc_buffer(net_adapter_t *adapter, void *buffer, void *data, unsigned long length)
//{
//	net_buffer_t *packet = (net_buffer_t *)exos_fifo_wait(&_free_wrappers, EXOS_TIMEOUT_NEVER);
//	packet->Node = (node_t) { .Type = EXOS_NODE_IO_BUFFER };	// FIXME
//	packet->Adapter = adapter;
//	packet->Buffer = buffer;
//	packet->Offset = (unsigned short)(data - buffer);
//	packet->Length = length;
//	return packet;
//}

void net_adapter_discard_input(net_buffer_t *buf)
{
	ASSERT(buf != NULL, KERNEL_ERROR_NULL_POINTER);
	net_adapter_t *adapter = buf->Adapter;
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	const net_driver_t *driver = adapter->Driver;
	ASSERT(driver != NULL && driver->DiscardInputBuffer != NULL, KERNEL_ERROR_NULL_POINTER);
	driver->DiscardInputBuffer(adapter, buf->Buffer);

//	exos_fifo_queue(&_free_wrappers, (node_t *)packet);
}

