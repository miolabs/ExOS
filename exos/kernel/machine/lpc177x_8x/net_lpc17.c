// LPC17xx Internal Ethernet Driver
// by Miguel Fides

#include "net_lpc17.h"
#include <support/lpc17/emac.h>

static void _input_handler(void *state);
static int _reset(NET_ADAPTER *adapter);
static void _link_up(NET_ADAPTER *adapter);
static void _link_down(NET_ADAPTER *adapter);
static void *_get_input_buffer(NET_ADAPTER *adapter, unsigned long *plength);
static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer);
static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size);
static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state);

const NET_DRIVER __net_driver_lpc17 = { 
	.Initialize = _reset,
	.LinkUp = _link_up,
	.LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer,
	.DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer,
	.SendOutputBuffer = _send_output_buffer };

static int _reset(NET_ADAPTER *adapter)
{
	return emac_initialize((ETH_MAC *)&adapter->MAC, _input_handler, adapter);
}

static void _link_up(NET_ADAPTER *adapter)
{
	ETH_LINK link = emac_init_link();
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
	void *buffer = emac_get_input_buffer(plength);
	exos_mutex_unlock(&adapter->InputLock);
	return buffer;
}

static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer)
{
	exos_mutex_lock(&adapter->InputLock);
	emac_discard_input(buffer);
	exos_mutex_unlock(&adapter->InputLock);
}

static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size)
{
	exos_mutex_lock(&adapter->OutputLock);
	void *buffer = emac_get_output_buffer(size);
	exos_mutex_unlock(&adapter->OutputLock);
	return buffer;
}

static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state)
{
	exos_mutex_lock(&adapter->OutputLock);
	int done = emac_send_output(mbuf, callback, state);
	exos_mutex_unlock(&adapter->OutputLock);
	return done;
}

static void _input_handler(void *state)
{
	NET_ADAPTER *adapter = (NET_ADAPTER *)state;
	if (adapter != NULL)
		exos_signal_set(&adapter->Thread, 1 << adapter->InputSignal);
}






