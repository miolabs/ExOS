#include "can_receiver.h"
#include <kernel/dispatch.h>

#ifndef CAN_RECEIVER_BUFFERS
#define CAN_RECEIVER_BUFFERS 10
#endif

static char _initialized = 0;
static char _enable_rx = 0;
static EXOS_LIST _handlers;
static EXOS_FIFO _can_free_msgs;
static CAN_RX_MSG _can_msg[CAN_RECEIVER_BUFFERS];
static int _lost_msgs = 0;

void can_receiver_initialize()
{
	if (!_initialized)
	{
		_initialized = 1;
		exos_fifo_create(&_can_free_msgs, NULL);
		for(int i = 0; i < CAN_RECEIVER_BUFFERS; i++) 
			exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);
	
		list_initialize(&_handlers);
	}
}

int can_receiver_add_handler(CAN_HANDLER *handler, int bus, unsigned long id, unsigned long id_mask)
{
	handler->Bus = bus;
	handler->Id = id;
	handler->IdMask = id_mask;
	exos_port_create(&handler->RxPort, NULL);
	_enable_rx = 0;
	list_add_tail(&_handlers, (EXOS_NODE *)handler);
	_enable_rx = 1;
}

static void _recycle_message(CAN_RX_MSG *rx_msg)
{
	exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)rx_msg);
}

int can_receiver_read(CAN_HANDLER *handler, CAN_MSG *msg, int timeout)
{
	CAN_RX_MSG *rx_msg = (CAN_RX_MSG *)exos_port_wait_message(&handler->RxPort, timeout);
	if (rx_msg != NULL)
	{
		*msg = rx_msg->CanMsg;
		_recycle_message(rx_msg);
		return 1;
	}
	return 0;
}

int can_receiver_get(CAN_HANDLER *handler, CAN_MSG *msg)
{
	CAN_RX_MSG *rx_msg = (CAN_RX_MSG *)exos_port_get_message(&handler->RxPort);
	if (rx_msg != NULL)
	{
		*msg = rx_msg->CanMsg;
		_recycle_message(rx_msg);
		return 1;
	}
	return 0;
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	if (_enable_rx)
	{
		CAN_HANDLER *matching = NULL;
		FOREACH(node, &_handlers)
		{
			CAN_HANDLER *handler = (CAN_HANDLER *)node;
			if (msg->EP.Bus == handler->Bus &&
				(msg->EP.Id & handler->IdMask) == handler->Id)
			{
				CAN_RX_MSG *rx_msg = (CAN_RX_MSG *)exos_fifo_dequeue(&_can_free_msgs);
				if (rx_msg != NULL)
				{
					rx_msg->CanMsg = *msg;
					exos_port_send_message(&handler->RxPort, (EXOS_MESSAGE *)rx_msg);
				}
				else can_receiver_msg_lost(index, msg);
			}
		}
	}
}

__weak
void can_receiver_msg_lost(int index, CAN_MSG *msg)
{
	_lost_msgs++;
}



