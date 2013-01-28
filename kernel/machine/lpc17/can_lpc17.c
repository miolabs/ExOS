#include "can_lpc17.h"
#include <kernel/fifo.h>

static int _reset(NET_ADAPTER *adapter);
static void _link_up(NET_ADAPTER *adapter);
static void _link_down(NET_ADAPTER *adapter);
static void *_get_input_buffer(NET_ADAPTER *adapter, unsigned long *plength);
static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer);
static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size);
static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state);

const NET_DRIVER __can_driver_lpc17 = { 
	.Initialize = _reset,
	.LinkUp = _link_up,
	.LinkDown = _link_down,
	.GetInputBuffer = _get_input_buffer,
	.DiscardInputBuffer = _discard_input_buffer,
	.GetOutputBuffer = _get_output_buffer,
	.SendOutputBuffer = _send_output_buffer };

#define CAN_BUFFERS 16
static EXOS_FIFO _free_fifo;
static EXOS_FIFO _input_fifo;
static EXOS_FIFO _output_fifo;
static CAN_BUFFER _free_buffers[CAN_BUFFERS];

static int _reset(NET_ADAPTER *adapter)
{
	exos_fifo_create(&_free_fifo, NULL);
	exos_fifo_create(&_input_fifo, NULL);
	exos_fifo_create(&_output_fifo, NULL);
	adapter->Speed = 1000;	// kbps
	return hal_can_initialize(0, 1000000);
}

static void _link_up(NET_ADAPTER *adapter)
{
//	adapter->Speed = 1000000;
}

static void _link_down(NET_ADAPTER *adapter)
{
}

static void *_get_input_buffer(NET_ADAPTER *adapter, unsigned long *plength)
{
//	exos_mutex_lock(&adapter->InputLock);
//	unsigned long length;
//	void *buffer = emac_get_input_buffer(plength);
//	exos_mutex_unlock(&adapter->InputLock);
//	return buffer;
}

static void _discard_input_buffer(NET_ADAPTER *adapter, void *buffer)
{
//	exos_mutex_lock(&adapter->InputLock);
//	emac_discard_input(buffer);
//	exos_mutex_unlock(&adapter->InputLock);
}

static void *_get_output_buffer(NET_ADAPTER *adapter, unsigned long size)
{
//	exos_mutex_lock(&adapter->OutputLock);
//	void *buffer = emac_get_output_buffer(size);
//	exos_mutex_unlock(&adapter->OutputLock);
//	return buffer;
}

static int _send_output_buffer(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state)
{
//	exos_mutex_lock(&adapter->OutputLock);
//	int done = emac_send_output(mbuf, callback, state);
//	exos_mutex_unlock(&adapter->OutputLock);
//	return done;
}

static void _input_handler(void *state)
{
	NET_ADAPTER *adapter = (NET_ADAPTER *)state;
	if (adapter != NULL)
		exos_signal_set(&adapter->Thread, 1 << adapter->InputSignal);
}

