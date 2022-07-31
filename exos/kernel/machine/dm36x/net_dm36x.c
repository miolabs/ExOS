// DM36X net driver
// by Miguel Fides

#include "net_dm36x.h"
#include <support/dm36x/emac.h>
#include <support/dm36x/emac_mdio.h>
#include <kernel/fifo.h>

#define ENET_INT_PRIORITY 11

static int _reset(NET_ADAPTER *adapter);
static void _link_up(NET_ADAPTER *adapter);
static void _link_down(NET_ADAPTER *adapter);
static void *_get_input_buffer(NET_ADAPTER *adapter, unsigned long *plength);
static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer);
static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size);
static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state);

const NET_DRIVER __net_driver_dm36x = { 
	.Initialize = _reset,
	.LinkUp = _link_up,
	.LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer,
	.DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer,
	.SendOutputBuffer = _send_output_buffer };

static NET_ADAPTER *_adapter = NULL;

static int _reset(NET_ADAPTER *adapter)
{
	_adapter = adapter;
	emac_initialize((unsigned char *)&adapter->MAC);
	return 1;
}

static void _link_up(NET_ADAPTER *adapter)
{
	ETH_LINK link = emac_check_link();
	if (link != ETH_LINK_NONE)
	{
		adapter->Speed = link & ETH_LINK_100M ? 100 : 10;
	}
}

static void _link_down(NET_ADAPTER *adapter)
{
	// TODO
}

static void *_get_input_buffer(NET_ADAPTER *adapter, unsigned long *plength)
{
	exos_mutex_lock(&adapter->InputLock);
	unsigned long length;
	ETH_HEADER *buffer = (ETH_HEADER *)emac_get_input_buffer(plength);
	exos_mutex_unlock(&adapter->InputLock);
	return buffer;
}

static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer)
{
	exos_mutex_lock(&adapter->InputLock);
	emac_discard_input_buffer(buffer);
	exos_mutex_unlock(&adapter->InputLock);
}

static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size)
{
	exos_mutex_lock(&adapter->OutputLock);
	ETH_HEADER *buffer = (ETH_HEADER *)emac_get_output_buffer(size);
	exos_mutex_unlock(&adapter->OutputLock);
	return buffer;
}

static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state)
{
	exos_mutex_lock(&adapter->OutputLock);
	int done = emac_send_output_buffer(mbuf, callback, state);
	exos_mutex_unlock(&adapter->OutputLock);
	return done;
}

void emac_dm36x_rx_handler()
{
	if (_adapter != NULL)
		exos_signal_set(&_adapter->Thread, 1 << _adapter->InputSignal);
}







