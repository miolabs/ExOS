#include <kernel/tests/threading_tests.h>
#include <kernel/tests/memory_tests.h>
#include <kernel/posix/tests/basic_pthread_tests.h>
#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <kernel/port.h>

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

void main()
{
	// enable CAN term
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;

	LPC_GPIO2->DIR |= RELAY0 | RELAY1;
	LPC_GPIO2->MASKED_ACCESS[RELAY0 | RELAY1] = 0;

	exos_port_create(&_port, NULL);
	exos_fifo_create(&_free_msgs, NULL);
	for(int i = 0; i < MSG_QUEUE; i++) exos_fifo_queue(&_free_msgs, (EXOS_NODE *)&_msg[i]);

	hal_can_initialize(0, 250000);
	hal_fullcan_setup(_can_setup, NULL);

	while(1)
	{
		RELAY_MSG *msg = (RELAY_MSG *)exos_port_get_message(&_port, 1000);
		if (msg != NULL)
		{
        	LPC_GPIO2->MASKED_ACCESS[RELAY0] = (msg->RelayMask & 1<<0) ? RELAY0 : 0;
			LPC_GPIO2->MASKED_ACCESS[RELAY1] = (msg->RelayMask & 1<<1) ? RELAY1 : 0;

			exos_thread_sleep(msg->Time);

			LPC_GPIO2->MASKED_ACCESS[RELAY0 | RELAY1] = 0;

            exos_fifo_queue(&_free_msgs, (EXOS_NODE *)msg);
		}
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
