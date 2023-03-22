#ifndef NET_NET_HUB_H
#define NET_NET_HUB_H

#include <net/adapter.h>
#include <kernel/fifo.h> 
#include <kernel/dispatch.h> 

typedef struct
{
	net_adapter_t HubAdapter[2];

	dispatcher_t InputDispatcher[2];
} net_hub_t;

extern const net_driver_t __net_hub_driver;

typedef struct
{
	fifo_t InputFifo;
	fifo_t *OutputFifo;
} hub_adapter_data_t;

bool net_hub_create(net_hub_t *br);

#endif // NET_NET_HUB_H


