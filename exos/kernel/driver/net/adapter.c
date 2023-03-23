#include "adapter.h"
#include "board.h"
#include "mbuf.h"
#include "net_service.h"
#include <support/misc/pools.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "net_adapter", __VA_ARGS__)

static mutex_t _adapters_lock;
static list_t _adapters;
static pool_t _buffer_pool;
static dispatcher_context_t *_context;

#define BUFFER_MAX_FRAME_SIZE ETH_MAX_FRAME_SIZE
#define POOL_ITEM_SIZE (sizeof(net_buffer_t) + BUFFER_MAX_FRAME_SIZE)
#define POOL_ITEM_COUNT 16
static unsigned char _pool_buffer[POOL_ITEM_COUNT * POOL_ITEM_SIZE];

static event_t _flush_event;
//static dispatcher_t _flush_dispatcher;

static void _flush(dispatcher_context_t *context, dispatcher_t *dispatcher);

void net_adapter_initialize()
{
	list_initialize(&_adapters);
	pool_create(&_buffer_pool, (node_t *)_pool_buffer, POOL_ITEM_SIZE, POOL_ITEM_COUNT);
	exos_mutex_create(&_adapters_lock);

/*
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
*/
}

static void _flush(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	_verbose(VERBOSE_ERROR, "automatic flush event!");
	net_adapter_flush(NULL);
	exos_dispatcher_add(context, dispatcher, EXOS_TIMEOUT_NEVER);
}

bool net_adapter_create(net_adapter_t *adapter, const net_driver_t *driver, unsigned phy_unit, const phy_handler_t *handler)
{
	ASSERT(adapter != NULL && driver != NULL, KERNEL_ERROR_NULL_POINTER);
	*adapter = (net_adapter_t) { .Node = (node_t) { .Type = EXOS_NODE_IO_DEVICE },
		.Driver = driver, .Context = _context };

	exos_mutex_create(&adapter->InputLock);
	exos_mutex_create(&adapter->OutputLock);

	exos_event_create(&adapter->InputEvent, EXOS_EVENTF_AUTORESET);
	bool done = driver->Initialize(adapter, phy_unit, handler);
	if (done)
	{
		ASSERT(adapter->Name != NULL, KERNEL_ERROR_NULL_POINTER);
		ASSERT(adapter->MaxFrameSize != 0, KERNEL_ERROR_NULL_POINTER);
	}
	return done;
}

void net_adapter_install(net_adapter_t *adapter, bool enable_network)
{
	ASSERT(adapter != NULL && adapter->Driver != NULL, KERNEL_ERROR_NULL_POINTER);
	exos_mutex_lock(&_adapters_lock);

	ASSERT(!list_find_node(&_adapters, &adapter->Node), KERNEL_ERROR_KERNEL_PANIC);
	list_add_tail(&_adapters, &adapter->Node);

	if (enable_network)
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

bool net_adapter_enum(net_adapter_t **padapter)
{
	net_adapter_t *adapter = *padapter;
#ifdef DEBUG
	if (adapter != NULL &&
		NULL == list_find_node(&_adapters, &adapter->Node))
	{
		*padapter = NULL;
		return false;
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

net_buffer_t *net_adapter_alloc_buffer(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
   	const net_driver_t *driver = adapter->Driver;
	ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned maxframesize = adapter->MaxFrameSize;
	ASSERT(maxframesize != 0 && maxframesize <= BUFFER_MAX_FRAME_SIZE, KERNEL_ERROR_KERNEL_PANIC);
	net_buffer_t *buf = (net_buffer_t *)pool_allocate(&_buffer_pool);
	if (buf != NULL)
	{
#ifdef DEBUG
		buf->Node = (node_t) { /* all zero */ };
#endif
		buf->Adapter = adapter;
		net_mbuf_init(&buf->Root, (unsigned char *)buf + sizeof(net_buffer_t), 0, maxframesize);
	}
	else
	{
		exos_event_set(&_flush_event);
	}
	return buf;
}

void net_adapter_free_buffer(net_buffer_t *buf)
{
	ASSERT(buf != NULL, KERNEL_ERROR_NULL_POINTER);

	bool done = false;
	if (buf->Adapter != NULL)
	{
		const net_driver_t *driver = buf->Adapter->Driver;
		ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
		if (driver->FreeBuffer != NULL)
			done = driver->FreeBuffer(buf->Adapter, buf);
	}
	if (!done)
		pool_free(&_buffer_pool, &buf->Node);
}

void net_adapter_flush(net_adapter_t *adapter)
{
	if (adapter != NULL)
	{
		const net_driver_t *driver = adapter->Driver;
		ASSERT(driver != NULL, KERNEL_ERROR_NULL_POINTER);
		if (driver->Flush != NULL)
			driver->Flush(adapter);
	}
	else
	{
		exos_mutex_lock(&_adapters_lock);
		net_adapter_t *adi = NULL;
		while(net_adapter_enum(&adi))
		{
			net_adapter_flush(adi);
		}
		exos_mutex_unlock(&_adapters_lock);	
	}
}

net_buffer_t *net_adapter_get_input(net_adapter_t *adapter)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	const net_driver_t *driver = adapter->Driver;
	ASSERT(driver != NULL && driver->GetInputBuffer != NULL, KERNEL_ERROR_NULL_POINTER);
	net_buffer_t *buf = driver->GetInputBuffer(adapter);
	return buf;
}

bool net_adapter_send_output(net_adapter_t *adapter, net_buffer_t *buf)
{
	ASSERT(adapter != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(buf != NULL, KERNEL_ERROR_NULL_POINTER);
	const net_driver_t *driver = adapter->Driver;
	ASSERT(driver != NULL && driver->SendOutputBuffer != NULL, KERNEL_ERROR_NULL_POINTER);
	bool done = driver->SendOutputBuffer(adapter, buf);
	return done;
}


__weak
void net_service_start(net_adapter_t *adapter)
{
	_verbose(VERBOSE_ERROR, "adapter '%s' added without net service!", adapter->Name);
}
