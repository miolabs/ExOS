#include "can_comms.h"
#include <support/can_hal.h>
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
	XCPU_MSG *xmsg = (XCPU_MSG *)exos_port_get_message(&_can_rx_port, 0);
	return xmsg;
}

void xcpu_can_release_message(XCPU_MSG *xmsg)
{
	if (xmsg != NULL)
		exos_fifo_queue(&_can_free_msgs, (EXOS_NODE *)xmsg);
}

// Sends "large_message_buffer", 128 bytes lenght max.
// Generic tool to send buffers to the lcd. Takes lots of time
static unsigned char _large_message_buffer[128];
static short _large_message_len = 0;
static short _large_message_cnt = 0;

static void _send_large_msg (int id1, int id2)
{
	CAN_BUFFER buf;
	int seg = _large_message_cnt >> 1;	// Repeat each part 2 times
	int seg0 = seg * 2;
	int seg1 = seg * 2 + 1;
	buf.u8[0] = seg0 | ((_large_message_len <= 7) ? 0x80 : 0); // Mark end of message
	for(int i=0; i<7; i++) buf.u8[1+i] = _large_message_buffer[seg0 * 7 + i];
	int done = hal_can_send((CAN_EP) { .Id =id1 }, &buf, 8, CANF_NONE);
	if (!done) 
		hal_can_cancel_tx();
	buf.u8[0] = seg1 | ((_large_message_len <= 14) ? 0x80 : 0); // Mark end of message
	for(int i=0; i<7; i++) buf.u8[1+i] = _large_message_buffer[seg1 * 7 + 7 + i];
	done = hal_can_send((CAN_EP) { .Id = id2 }, &buf, 8, CANF_NONE);
	if (!done) 
		hal_can_cancel_tx();

	if((_large_message_cnt & 0x3) == 3)	// Discount on each 4th repeated sending
		_large_message_len -= 14;
	_large_message_cnt++;
}

static void _load_large_msg ( int len)
{
	if(_large_message_len > 0)
		return; // If previous msg not finished, fail
	if (len > 128)
		len = 128;
	_large_message_cnt = 0;
	_large_message_len = len;
}

void xcpu_can_send_messages(XCPU_MASTER_OUT1 *report, XCPU_MASTER_OUT2 *adj, 
							XCPU_MASTER_OUT3 *cust)
{
	int done;
	int i;
	CAN_BUFFER buf;

	// Large message system
	if (_large_message_len > 0)
		_send_large_msg (0x303,0x304);

#if 0	// Test
	static int test_large = 0;
	if (test_large == 0)
	{
		for(int i=0; i<100; i++)
			_large_message_buffer[i] = 'a' + (i & 0x1f);
		test_large = 1;
		_large_message_len = 100;
	}
	if(_large_message_len <= 0) 
		_load_large_msg ( 100);
#endif

	if (cust)
	{
		for(i = 0; i< 7; i++)
			buf.u8[i] = cust->Curve[i];
		int done = hal_can_send((CAN_EP) { .Id = 0x302 }, &buf, 8, CANF_NONE);
		if (!done) 
			hal_can_cancel_tx();
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

