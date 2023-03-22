#ifndef NET_SERVICE_H
#define NET_SERVICE_H

#include <net/adapter.h>
#include <kernel/dispatch.h>

typedef struct
{
	node_t Node;
	eth_type_t Protocol;
} net_config_t;

typedef struct
{
	net_adapter_t *Adapter;
	dispatcher_t InputDispatcher;
	list_t ConfigList;
} net_wrapper_t;

void net_service_start(net_adapter_t *adapter);
bool net_service_get_config(net_adapter_t *adapter, unsigned protocol, net_config_t **pconfig);
void net_service_set_config(net_adapter_t *adapter, net_config_t *config);
void *net_output(net_adapter_t *adapter, net_buffer_t *buf, unsigned hdr_size, const hw_addr_t *destination, eth_type_t type);
void net_input(net_adapter_t *adapter);


#endif // NET_SERVICE_H

