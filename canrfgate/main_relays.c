#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <kernel/port.h>
#include <kernel/dispatch.h>

#include <support/can_hal.h>

#include "relay.h"

#define RELAY0 (1<<6)
#define RELAY1 (1<<7)
#define RELAY_COUNT 2

static const CAN_EP _eps[] = { {0x200, 0}, {0x201, 0} };
static int _can_setup(int index, CAN_EP *ep, CAN_MSG_FLAGS *pflags, void *state);

static EXOS_PORT _port;
static EXOS_FIFO _free_msgs;
#define MSG_QUEUE 1
static RELAY_MSG _msg[MSG_QUEUE];

static void _msg_rcv(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static void _relay_timeout(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher);
static EXOS_DISPATCHER _relay_done;

#define HEARTBEAT_TIMEOUT 1000
static unsigned short _current_mask = 0;
static unsigned short _pending_mask = 0;

void main()
{
	exos_port_create(&_port, NULL);
	exos_fifo_create(&_free_msgs, NULL);
	for(int i = 0; i < MSG_QUEUE; i++) exos_fifo_queue(&_free_msgs, (EXOS_NODE *)&_msg[i]);

	hal_can_initialize(0, 250000, CAN_INITF_DISABLE_RETRANSMISSION);
	hal_fullcan_setup(_can_setup, NULL);

	EXOS_DISPATCHER_CONTEXT context;
	exos_dispatcher_context_create(&context);
	EXOS_DISPATCHER msg_dispatcher = (EXOS_DISPATCHER) { .Callback = _msg_rcv, .Event = &_port.Event };
	exos_dispatcher_add(&context, &msg_dispatcher, HEARTBEAT_TIMEOUT);

	while(1)
	{
		exos_dispatch(&context, EXOS_TIMEOUT_NEVER);
	}
}

static void _set_relays(unsigned short mask)
{
	LPC_GPIO2->MASKED_ACCESS[RELAY0] = (mask & 1<<0) ? RELAY0 : 0;
	LPC_GPIO2->MASKED_ACCESS[RELAY1] = (mask & 1<<1) ? RELAY1 : 0;

	CAN_BUFFER buf = (CAN_BUFFER) { .u16[0] = mask };
	hal_can_send((CAN_EP) { .Id = 0x301 }, &buf, 2, CANF_PRI_ANY);
}

static void _msg_rcv(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	RELAY_MSG *msg = (RELAY_MSG *)exos_port_get_message(&_port);
	if (msg != NULL)
	{
		exos_dispatcher_remove(context, &_relay_done);	// NOTE: it's safe to remove it even if not added 

		_current_mask = (_pending_mask & ~msg->Mask) | (msg->Value & msg->Mask);
		_set_relays(_current_mask);

		if (msg->Time != 0)
		{
			_pending_mask = _current_mask & ~msg->Mask;	// turn off selected relays
			_relay_done = (EXOS_DISPATCHER) { .Callback = _relay_timeout, .CallbackState = NULL };
			exos_dispatcher_add(context, &_relay_done, msg->Time);
		}
		else
		{
			_pending_mask = _current_mask;
		}
		exos_fifo_queue(&_free_msgs, (EXOS_NODE *)msg);
	}

#ifdef DEBUG
	static unsigned long seq = 0;
	CAN_BUFFER buf = (CAN_BUFFER) { .u16[0] = _current_mask, .u16[1] = seq++ };
	hal_can_send((CAN_EP) { .Id = 0x300 }, &buf, 4, CANF_PRI_ANY);
#endif

	exos_dispatcher_add(context, dispatcher, HEARTBEAT_TIMEOUT); // re-issue this dispatcher
}

static void _relay_timeout(EXOS_DISPATCHER_CONTEXT *context, EXOS_DISPATCHER *dispatcher)
{
	_current_mask = _pending_mask;
	_set_relays(_current_mask);
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
				rmsg->Mask = msg->Data.u16[0];
				rmsg->Value = msg->Data.u16[1];
				rmsg->Time = msg->Data.u32[1];
				exos_port_send_message(&_port, (EXOS_MESSAGE *)rmsg);
			}
			break;
		case 0x201:
			// TODO
			break;
	}
}

void hal_board_initialize()
{
	// enable active CAN termination
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;

	LPC_GPIO2->DIR |= RELAY0 | RELAY1;
	LPC_GPIO2->MASKED_ACCESS[RELAY0 | RELAY1] = 0;
}
