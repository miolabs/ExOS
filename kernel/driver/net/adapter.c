#include "adapter.h"
#include "mbuf.h"
#include "arp.h"
#include "arp_tables.h"
#include "ip.h"
#include "net_service.h"
#include <kernel/fifo.h>
#include <kernel/panic.h>
#include <support/net_hal.h>
 
static EXOS_MUTEX _adapters_lock;
static EXOS_LIST _adapters;
static EXOS_FIFO _free_wrappers;
static EXOS_EVENT _free_wrapper_event;
#define NET_PACKET_WRAPPERS 16
static ETH_INPUT_BUFFER _wrappers[NET_PACKET_WRAPPERS];

static const HW_ADDR _dummy = { 1, 2, 3, 4, 5, 6 };

void net_adapter_initialize()
{
	list_initialize(&_adapters);
	exos_event_create(&_free_wrapper_event);
	exos_fifo_create(&_free_wrappers, &_free_wrapper_event);
	for(int i = 0; i < NET_PACKET_WRAPPERS; i++)
	{
		_wrappers[i] = (ETH_INPUT_BUFFER) { .Adapter = NULL }; 
		exos_fifo_queue(&_free_wrappers, (EXOS_NODE *)&_wrappers[i]);
	}

	exos_mutex_create(&_adapters_lock);
	ETH_ADAPTER *adapter;
	int index = 0;
	while(NULL != (adapter = hal_net_get_adapter(index)))
	{
#ifdef DEBUG
		adapter->Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_DEVICE };
#endif
		adapter->IP = (IP_ADDR) { .Value = 0 };
		adapter->NetMask = (IP_ADDR) { .Bytes = { 255, 255, 255, 255 } };
		adapter->Gateway = (IP_ADDR) { .Value = 0 };
		adapter->Speed = 0;
		hal_net_set_mac_address(adapter, index);

		const ETH_DRIVER *driver = adapter->Driver;
		if (driver->Initialize(adapter))
		{
			list_add_tail(&_adapters, (EXOS_NODE *)adapter);

			exos_mutex_create(&adapter->InputLock);
			exos_mutex_create(&adapter->OutputLock);
			net_service_start(adapter);

			hal_net_set_ip_address(adapter, index);
		}
		index++;
	}
}

__attribute__((__weak__))
void hal_net_set_mac_address(ETH_ADAPTER *adapter, int index)
{
	adapter->MAC = _dummy;
	adapter->MAC.Bytes[5] += index;
}

void net_adapter_list_lock()
{
	exos_mutex_lock(&_adapters_lock);
}

void net_adapter_list_unlock()
{
	exos_mutex_unlock(&_adapters_lock);
}

int net_adapter_enum(ETH_ADAPTER **padapter)
{
	ETH_ADAPTER *adapter = *padapter;
#ifdef DEBUG
	if (adapter != NULL &&
		NULL == list_find_node(&_adapters, (EXOS_NODE *)adapter))
	{
		*padapter = NULL;
		return 0;
	}
#endif

	adapter = (adapter == NULL) ? (!LIST_ISEMPTY(&_adapters) ? (ETH_ADAPTER *)(LIST_HEAD(&_adapters)->Succ) : NULL) : 
		(ETH_ADAPTER *)NODE_SUCC(&_adapters, (EXOS_NODE *)adapter);
	*padapter = adapter;
	return (adapter != NULL);
}

ETH_ADAPTER *net_adapter_find(IP_ADDR addr)
{
	FOREACH(node, &_adapters)
	{
		ETH_ADAPTER *adapter = (ETH_ADAPTER *)node;
#ifdef DEBUG
		if (adapter->Node.Type != EXOS_NODE_IO_DEVICE)
			kernel_panic(KERNEL_ERROR_WRONG_NODE);
#endif
		IP_ADDR network = (IP_ADDR) { .Value = (adapter->IP.Value & adapter->NetMask.Value) };
		if (network.Value == (addr.Value & adapter->NetMask.Value) ||
			addr.Value == 0)
			return adapter;
	}
	return NULL;
}

void net_adapter_input(ETH_ADAPTER *adapter)
{
   	const ETH_DRIVER *driver = adapter->Driver;

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
		}
		if (!queued) 
			driver->DiscardInputBuffer(adapter, buffer);
	}
}

void *net_adapter_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output, unsigned hdr_size, HW_ADDR *destination, ETH_TYPE type)
{
	const ETH_DRIVER *driver = adapter->Driver;

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

int net_adapter_send_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *output)
{
	const ETH_DRIVER *driver = adapter->Driver;
	int done = driver->SendOutputBuffer(adapter, &output->Buffer, 
		output->CompletedEvent != NULL ? _send_callback : NULL, output->CompletedEvent);
	return done;
}

ETH_INPUT_BUFFER *net_adapter_alloc_input_buffer(ETH_ADAPTER *adapter, void *buffer)
{
	ETH_INPUT_BUFFER *packet = (ETH_INPUT_BUFFER *)exos_fifo_wait(&_free_wrappers, EXOS_TIMEOUT_NEVER);
#ifdef DEBUG
	packet->Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_BUFFER };
#endif
	packet->Adapter = adapter;
	packet->Buffer = buffer;
	return packet;
}

void net_adapter_discard_input_buffer(ETH_INPUT_BUFFER *packet)
{
	ETH_ADAPTER *adapter = packet->Adapter;
	const ETH_DRIVER *driver = adapter->Driver;
	driver->DiscardInputBuffer(adapter, packet->Buffer);

	exos_fifo_queue(&_free_wrappers, (EXOS_NODE *)packet);
}

