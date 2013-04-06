#include "adapter.h"
#include "board.h"
#include "mbuf.h"
#include "arp.h"
#include "arp_tables.h"
#include "ip.h"
#include "net_service.h"
#include <kernel/fifo.h>
#include <kernel/panic.h>
 
static EXOS_MUTEX _adapters_lock;
static EXOS_LIST _adapters;
static EXOS_FIFO _free_wrappers;
static EXOS_EVENT _free_wrapper_event;
#define NET_PACKET_WRAPPERS 16
static NET_BUFFER _wrappers[NET_PACKET_WRAPPERS];

void net_adapter_initialize()
{
	list_initialize(&_adapters);
	exos_event_create(&_free_wrapper_event);
	exos_fifo_create(&_free_wrappers, &_free_wrapper_event);
	for(int i = 0; i < NET_PACKET_WRAPPERS; i++)
	{
		_wrappers[i] = (NET_BUFFER) { .Adapter = NULL }; 
		exos_fifo_queue(&_free_wrappers, (EXOS_NODE *)&_wrappers[i]);
	}

	exos_mutex_create(&_adapters_lock);
	NET_ADAPTER *adapter;
	int index = 0;
	while(NULL != (adapter = net_board_get_adapter(index)))
	{
#ifdef DEBUG
		adapter->Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_DEVICE };
#endif
		adapter->IP = (IP_ADDR) { .Value = 0 };
		adapter->NetMask = (IP_ADDR) { .Bytes = { 255, 255, 255, 255 } };
		adapter->Gateway = (IP_ADDR) { .Value = 0 };
		adapter->Speed = 0;
		net_board_set_mac_address(adapter, index);

		const NET_DRIVER *driver = adapter->Driver;
		if (driver->Initialize(adapter))
		{
			list_add_tail(&_adapters, (EXOS_NODE *)adapter);

			exos_mutex_create(&adapter->InputLock);
			exos_mutex_create(&adapter->OutputLock);
			net_service_start(adapter);

			net_board_set_ip_address(adapter, index);
		}
		index++;
	}
}

void net_adapter_list_lock()
{
	exos_mutex_lock(&_adapters_lock);
}

void net_adapter_list_unlock()
{
	exos_mutex_unlock(&_adapters_lock);
}

int net_adapter_enum(NET_ADAPTER **padapter)
{
	NET_ADAPTER *adapter = *padapter;
#ifdef DEBUG
	if (adapter != NULL &&
		NULL == list_find_node(&_adapters, (EXOS_NODE *)adapter))
	{
		*padapter = NULL;
		return 0;
	}
#endif

	adapter = (adapter == NULL) ? (!LIST_ISEMPTY(&_adapters) ? (NET_ADAPTER *)(LIST_HEAD(&_adapters)->Succ) : NULL) : 
		(NET_ADAPTER *)NODE_SUCC(&_adapters, (EXOS_NODE *)adapter);
	*padapter = adapter;
	return (adapter != NULL);
}

NET_ADAPTER *net_adapter_find(IP_ADDR addr)
{
	FOREACH(node, &_adapters)
	{
		NET_ADAPTER *adapter = (NET_ADAPTER *)node;
#ifdef DEBUG
		if (adapter->Node.Type != EXOS_NODE_IO_DEVICE)
			kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
		IP_ADDR network = (IP_ADDR) { .Value = (adapter->IP.Value & adapter->NetMask.Value) };
		if (network.Value == (addr.Value & adapter->NetMask.Value))
			return adapter;
	}
	return NULL;
}

void net_adapter_input(NET_ADAPTER *adapter)
{
#ifdef DEBUG
	if (adapter == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
   	const NET_DRIVER *driver = adapter->Driver;

	while(1)
	{
		unsigned long length;
		ETH_HEADER *buffer = driver->GetInputBuffer(adapter, &length);
		if (buffer == NULL) break;

		ETH_TYPE packet_type = NTOH16(buffer->Type);
		void *payload = (void *)buffer + sizeof(ETH_HEADER);

		int queued = 0;
		switch(packet_type)
		{
			case ETH_TYPE_ARP:
				net_arp_input(adapter, (ARP_HEADER *)payload);
				break;
			case ETH_TYPE_IP:
				queued = net_ip_input(adapter, buffer, (IP_HEADER *)payload);
				break;
		}
		if (!queued) 
			driver->DiscardInputBuffer(adapter, buffer);
	}
}

void *net_adapter_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, unsigned hdr_size, HW_ADDR *destination, ETH_TYPE type)
{
#ifdef DEBUG
	if (adapter == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif
	const NET_DRIVER *driver = adapter->Driver;

	hdr_size += sizeof(ETH_HEADER);
	ETH_HEADER *buffer = driver->GetOutputBuffer(adapter, hdr_size);
	if (buffer != NULL)
	{
		buffer->Sender = adapter->MAC;
		buffer->Destination = *destination;
		buffer->Type = HTON16(type);
		
		// initialize mbuf
		net_mbuf_init(&output->Buffer, buffer, 0, hdr_size);

		return (void *)buffer + sizeof(ETH_HEADER);
	}
	return NULL;
}

static void _send_callback(void *state)
{
	EXOS_EVENT *event = (EXOS_EVENT *)state;
	if (event == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	
	exos_event_set(event);
}

int net_adapter_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output)
{
	const NET_DRIVER *driver = adapter->Driver;
	int done = driver->SendOutputBuffer(adapter, &output->Buffer, 
		output->CompletedEvent != NULL ? _send_callback : NULL, output->CompletedEvent);
	return done;
}

NET_BUFFER *net_adapter_alloc_buffer(NET_ADAPTER *adapter, void *buffer, void *data, unsigned long length)
{
	NET_BUFFER *packet = (NET_BUFFER *)exos_fifo_wait(&_free_wrappers, EXOS_TIMEOUT_NEVER);
#ifdef DEBUG
	packet->Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_BUFFER };
#endif
	packet->Adapter = adapter;
	packet->Buffer = buffer;
	packet->Offset = (unsigned short)(data - buffer);
	packet->Length = length;
	return packet;
}

void net_adapter_discard_input_buffer(NET_BUFFER *packet)
{
	NET_ADAPTER *adapter = packet->Adapter;
	const NET_DRIVER *driver = adapter->Driver;
	driver->DiscardInputBuffer(adapter, packet->Buffer);

	exos_fifo_queue(&_free_wrappers, (EXOS_NODE *)packet);
}

