#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/dispatch.h>

#include <support/can_hal.h>

#include "relay.h"

#define RELAY0 (1<<6)
#define RELAY1 (1<<7)

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };
static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static EXOS_PORT _port;
static EXOS_FIFO _free_msgs;
#define MSG_QUEUE 1
static RELAY_MSG _msg[MSG_QUEUE];

static void _msg_rcv(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static void _relay_timeout(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static EXOS_DISPATCHER _relay_done;

void main()
{
	// enable active CAN termination
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;

	LPC_GPIO2->DIR |= RELAY0 | RELAY1;
	LPC_GPIO2->MASKED_ACCESS[RELAY0 | RELAY1] = 0;

	exos_port_create(&_port, NULL);
	exos_fifo_create(&_free_msgs, NULL);
	for(int i = 0; i < MSG_QUEUE; i++) exos_fifo_queue(&_free_msgs, (EXOS_NODE *)&_msg[i]);

	hal_can_initialize(0, 250000);
	hal_fullcan_setup(_can_setup, NULL);

	EXOS_DISPATCHER_CONTEXT context;
	exos_dispatcher_context_create(&context);
	EXOS_DISPATCHER msg_dispatcher = (EXOS_DISPATCHER) { .Callback = _msg_rcv, .Event = &_port.Event };
	exos_dispatcher_add(&context, &msg_dispatcher, 1000);	// NOTE: will timeout every second

	while(1)
	{
		exos_dispatch(&context, EXOS_TIMEOUT_NEVER);
	}
}

static void _msg_rcv(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	RELAY_MSG *msg = (RELAY_MSG *)exos_port_get_message(&_port, 0);
	if (msg != NULL)
	{
		exos_dispatcher_remove(context, &_relay_done);	// NOTE: it's safe to remove it even if not added 

		LPC_GPIO2->MASKED_ACCESS[RELAY0] = (msg->RelayMask & 1<<0) ? RELAY0 : 0;
		LPC_GPIO2->MASKED_ACCESS[RELAY1] = (msg->RelayMask & 1<<1) ? RELAY1 : 0;

		_relay_done = (EXOS_DISPATCHER) { .Callback = _relay_timeout, .CallbackState = (void *)msg->RelayMask };
		exos_dispatcher_add(context, &_relay_done, 1000);

		exos_fifo_queue(&_free_msgs, (EXOS_NODE *)msg);
	}

	CAN_BUFFER buf = (CAN_BUFFER) { 0, 1, 2, 3 };
	hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 4, CANF_PRI_ANY); 

	exos_dispatcher_add(context, dispatcher, 1000); // re-issue this dispatcher
}

static void _relay_timeout(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
		LPC_GPIO2->MASKED_ACCESS[RELAY0] = 0;
		LPC_GPIO2->MASKED_ACCESS[RELAY1] = 0;
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

void hal_can_received_handler(int index, CAN_MSG *msg)
{
	RELAY_MSG *rmsg;
	switch(msg->EP.Id)
	{
		case 0x200:
			rmsg = (RELAY_MSG *)exos_fifo_dequeue(&_free_msgs);
			if (rmsg != NULL)
			{
				rmsg->Time = msg->Data.u32[1];
				rmsg->RelayMask = msg->Data.u32[0];
				exos_port_send_message(&_port, (EXOS_MESSAGE *)rmsg);
			}
			break;
		case 0x201:
			// TODO
			break;
	}
}
