// LPC17xx MAC Driver
// by Miguel Fides

#include "net_lpc17.h"
#include <support/lpc17/emac.h>
#include <kernel/fifo.h>

#define ENET_INT_PRIORITY 11

static void _input_handler();
static int _reset(ETH_ADAPTER *adapter);
static void _link_up(ETH_ADAPTER *adapter);
static void _link_down(ETH_ADAPTER *adapter);
static ETH_HEADER *_get_input_buffer(ETH_ADAPTER *adapter, unsigned long *plength);
static void _discard_input_buffer(ETH_ADAPTER *adapter, ETH_HEADER *buffer);
static ETH_HEADER *_get_output_buffer(ETH_ADAPTER *adapter, unsigned long size);
static int _send_output_buffer(ETH_ADAPTER *adapter, NET_MBUF *mbuf, ETH_CALLBACK callback, void *state);

const ETH_DRIVER __net_driver_lpc17 = { 
	.Initialize = _reset,
	.LinkUp = _link_up,
	.LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer,
	.DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer,
	.SendOutputBuffer = _send_output_buffer };

static ETH_ADAPTER *_adapter = NULL;

static int _reset(ETH_ADAPTER *adapter)
{
	_adapter = adapter;
	return emac_initialize((ETH_MAC *)&adapter->MAC, _input_handler);
}

static void _link_up(ETH_ADAPTER *adapter)
{
	ETH_LINK link = emac_init_link();
	if (link != ETH_LINK_NONE)
	{
		adapter->Speed = link & ETH_LINK_100M ? 100 : 10;
	}
}

static void _link_down(ETH_ADAPTER *adapter)
{
	// TODO
}

static ETH_HEADER *_get_input_buffer(ETH_ADAPTER *adapter, unsigned long *plength)
{
	exos_mutex_lock(&adapter->InputLock);
	unsigned long length;
	ETH_HEADER *buffer = (ETH_HEADER *)emac_get_input_buffer(plength);
	exos_mutex_unlock(&adapter->InputLock);
	return buffer;
}

static void _discard_input_buffer(ETH_ADAPTER *adapter, ETH_HEADER *buffer)
{
	exos_mutex_lock(&adapter->InputLock);
	emac_discard_input(buffer);
	exos_mutex_unlock(&adapter->InputLock);
}

static ETH_HEADER *_get_output_buffer(ETH_ADAPTER *adapter, unsigned long size)
{
	exos_mutex_lock(&adapter->OutputLock);
	ETH_HEADER *buffer = (ETH_HEADER *)emac_get_output_buffer(size);
	exos_mutex_unlock(&adapter->OutputLock);
	return buffer;
}

static int _send_output_buffer(ETH_ADAPTER *adapter, NET_MBUF *mbuf, ETH_CALLBACK callback, void *state)
{
	exos_mutex_lock(&adapter->OutputLock);
	int done = emac_send_output(mbuf, callback, state);
	exos_mutex_unlock(&adapter->OutputLock);
	return done;
}

static void _input_handler()
{
	if (_adapter != NULL)
		exos_signal_set(&_adapter->Thread, 1 << _adapter->InputSignal);
}






