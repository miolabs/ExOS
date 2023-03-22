#include "net_service.h"
#include "arp.h"
#include "arp_tables.h"
#include "ip.h"
#include <kernel/memory.h>
#include <kernel/thread.h>
#include <kernel/dispatch.h>
#include <kernel/panic.h>

static void *_service(void *arg);

static void _input_callback(dispatcher_context_t *context, dispatcher_t *dispacther);

#define NET_ADAPTER_THREAD_STACK 4096
static unsigned char _thread_stack[NET_ADAPTER_THREAD_STACK] __aligned(32);
static exos_thread_t _thread;
static bool _init_done = false;

static dispatcher_context_t _context;

void net_service_start(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);

	if (!_init_done)
	{
		event_t init;
		exos_event_create(&init, EXOS_EVENTF_AUTORESET);
		exos_thread_create(&_thread, 5, _thread_stack, NET_ADAPTER_THREAD_STACK, _service, &init);

		exos_event_wait(&init, EXOS_TIMEOUT_NEVER);
		_init_done = true;
	}

	ASSERT(adapter->NetData == NULL, KERNEL_ERROR_KERNEL_PANIC);
	net_wrapper_t *wrapper = (net_wrapper_t *)exos_mem_alloc(sizeof(net_wrapper_t), EXOS_MEMF_CLEAR);
	ASSERT(wrapper != NULL, KERNEL_ERROR_NOT_ENOUGH_MEMORY);
	adapter->NetData = wrapper;

	wrapper->Adapter = adapter;
	list_initialize(&wrapper->ConfigList);
	net_ip_init_config(adapter);

	exos_dispatcher_create(&wrapper->InputDispatcher, &adapter->InputEvent, _input_callback, wrapper);
	exos_dispatcher_add(&_context, &wrapper->InputDispatcher, EXOS_TIMEOUT_NEVER);
}

static void *_service(void *arg)
{
	event_t *init = (event_t *)arg;
	exos_dispatcher_context_create(&_context);
	exos_event_set(init);

	while(1)
	{
		exos_dispatch(&_context, EXOS_TIMEOUT_NEVER);
	}
}

static void _input_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	net_wrapper_t *wrapper = (net_wrapper_t *)dispatcher->CallbackState;
	ASSERT(wrapper != NULL, KERNEL_ERROR_NULL_POINTER);
	net_adapter_t *adapter = wrapper->Adapter;
	ASSERT(adapter != NULL && adapter->NetData == wrapper, KERNEL_ERROR_KERNEL_PANIC);

	const net_driver_t *driver = adapter->Driver;
	if (adapter->Speed != 0)
	{
		net_input(adapter);
	}
	else
	{
		driver->LinkUp(adapter);
	}

	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

bool net_service_get_config(net_adapter_t *adapter, unsigned protocol, net_config_t **pconfig)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	net_wrapper_t *wrapper = (net_wrapper_t *)adapter->NetData;
	
	bool done = false;
	if (wrapper != NULL)
	{
		ASSERT(wrapper != NULL && wrapper->Adapter == adapter, KERNEL_ERROR_KERNEL_PANIC); 
		FOREACH(n, &wrapper->ConfigList)
		{
			net_config_t *config = (net_config_t *)n;
			if (config->Protocol == protocol)
			{
				*pconfig = config;
				done = true;
				break;
			}
		}
	}
	return done;
}

void net_service_set_config(net_adapter_t *adapter, net_config_t *config)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	net_wrapper_t *wrapper = (net_wrapper_t *)adapter->NetData;
	ASSERT(wrapper != NULL && wrapper->Adapter == adapter, KERNEL_ERROR_KERNEL_PANIC); 
	
	ASSERT(!list_find_node(&wrapper->ConfigList, &config->Node), KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);
	list_add_tail(&wrapper->ConfigList, &config->Node);
}

void net_input(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
   	const net_driver_t *driver = adapter->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);

	net_buffer_t *buf;
	while(buf = net_adapter_get_input(adapter), buf != NULL)
	{
		eth_header_t *eth = (eth_header_t *)(buf->Root.Buffer + buf->Root.Offset);
		eth_type_t packet_type = NTOH16(eth->Type);
		void *payload = (void *)eth + sizeof(eth_header_t);

		int queued = 0;
		switch(packet_type)
		{
			case ETH_TYPE_ARP:
				queued = net_arp_input(adapter, (arp_header_t *)payload);
				break;
			case ETH_TYPE_IP:
				queued = net_ip_input(adapter, eth, (ip_header_t *)payload);
				break;
#ifdef ENABLE_ETHERCAT
			case ETH_TYPE_ETHERCAT:
				queued = net_ecat_input(adapter, buffer, (ECAT_HEADER *)payload);
				break;
#endif
		}
		if (!queued) 
		{
			//NOTE: API changed and you can't keep buffers and discard out-of-order so "queue" is not a good idea... [WIP]
			net_adapter_free_buffer(buf);
		}
	}
}

void *net_output(net_adapter_t *adapter, net_buffer_t *output, unsigned length, const hw_addr_t *destination, eth_type_t type)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	const net_driver_t *driver = adapter->Driver;
	ASSERT(output != NULL, KERNEL_ERROR_NULL_POINTER);

	length += sizeof(eth_header_t);
	ASSERT(length <= adapter->MaxFrameSize, KERNEL_ERROR_KERNEL_PANIC);

	net_mbuf_t *mbuf = &output->Root;
	eth_header_t *eth = (eth_header_t *)mbuf->Buffer + mbuf->Offset;	// NOTE: usually Offet is 0
	eth->Sender = adapter->MAC;
	eth->Destination = *destination;
	eth->Type = HTON16(type);
	
	mbuf->Length = length;
	return (void *)eth + sizeof(eth_header_t);
}



