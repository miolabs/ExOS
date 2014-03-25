#include "can_comms.h"
#include "../multipacket_msg.h"
#include "persist.h"
#include <support/can_hal.h>
#include <kernel/machine/hal.h>
#include <kernel/types.h>

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };
static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);
static EXOS_PORT _can_rx_port;
static EXOS_FIFO _can_free_msgs;
#define CAN_MSG_QUEUE 10
static XCPU_MSG _can_msg[CAN_MSG_QUEUE];
#ifdef DEBUG
static int _lost_msgs = 0;
#endif

void xcpu_can_initialize()
{
	exos_port_create(&_can_rx_port, NULL);
	exos_fifo_create(&_can_free_msgs, NULL);
	for(int i = 0; i < CAN_MSG_QUEUE; i++) exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)&_can_msg[i]);

	hal_can_initialize(0, 250000, 0); //CAN_INITF_DISABLE_RETRANSMISSION);
	hal_fullcan_setup(_can_setup, NULL);
}

XCPU_MSG *xcpu_can_get_message()
{
	XCPU_MSG *xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port);
	return xmsg;
}

void xcpu_can_release_message(XCPU_MSG *xmsg)
{
	if (xmsg != NULL)
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
}


void xcpu_can_send_messages(XCPU_MASTER_OUT1 *report, XCPU_MASTER_OUT2 *adj, 
							XCPU_PERSIST_DATA* storage)
{
	static int alternate_msg = 0;
	int done, i;
	CAN_BUFFER buf;

	// Alternate sending of multipacket messages
	int busy = multipacket_msg_send(0x302,0x303);
	if (!busy)
	{
		unsigned char* test_msg = multipacket_msg_reset(100);
		switch(alternate_msg)
		{
			case 0:
				test_msg[0] = XCPU_MULTI_PHONE_LOG;
				__mem_copy(&test_msg[1], &test_msg[1 +sizeof(storage->Phones)], storage->Phones);
				break;
			case 1:
				test_msg[0] = XCPU_MULTI_CUSTOM_CURVE;
				__mem_copy(&test_msg[1], &test_msg[1 +sizeof(char) * 7], storage->CustomCurve);
				break;
			default:
				break;
		}
		alternate_msg = (alternate_msg + 1) % 2;
	}

	if (adj != NULL)
	{
		done = hal_can_send((CAN_EP) { .Id = 0x301 }, (CAN_BUFFER *)adj, 8, CANF_NONE);
		if (!done) 
			hal_can_cancel_tx();
	}

	if (report != NULL)
	{
		done = hal_can_send((CAN_EP) { .Id = 0x300 }, (CAN_BUFFER *)report, 8, CANF_NONE);
		if (!done) 
			hal_can_cancel_tx();
	}
}

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	XCPU_MSG *xmsg;
	switch(msg->EP.Id)
	{
		case 0x200:                
		case 0x201:
		xmsg = (XCPU_MSG *)exos_fifo_dequeue(&_can_free_msgs);
		if (xmsg != NULL)
		{
			xmsg->CanMsg = *msg;
			exos_port_send_message(&_can_rx_port, (EXOS_MESSAGE *)xmsg);
		}
#ifdef DEBUG
		else _lost_msgs++;
#endif
		break;
	}
}

static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state)
{
	int count = sizeof(_eps) / sizeof(CAN_EP);
	if (index < count)
	{
		*ep = _eps[index];
		*pflags = CANF_RXINT;
		return 1;
	}
	return 0;
}

