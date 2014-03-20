#include <kernel/machine/hal.h>
#include <support/can_hal.h>
#include <kernel/types.h>
#include <assert.h>

#include "multipacket_msg.h"

// Sends "large_message_buffer", 128 bytes length max.
// Generic tool to send buffers to the lcd. Takes lots of time
static unsigned char _send_message_buffer[MULTIPACKET_MAX_LEN];
static short _send_message_len = 0;
static short _send_message_cnt = 0;

int multipacket_msg_send (int id1, int id2)
{
	if (_send_message_len > 0)
	{
		CAN_BUFFER buf;
		int seg = _send_message_cnt >> 1;	// Repeat each part 2 times
		int seg0 = seg * 2;
		int seg1 = seg * 2 + 1;
		buf.u8[0] = seg0 | ((_send_message_len <= 7) ? 0x80 : 0); // Mark end of message
		for(int i=0; i<7; i++) 
			buf.u8[1+i] = _send_message_buffer[seg0 * 7 + i];
		int done = hal_can_send((CAN_EP) { .Id =id1 }, &buf, 8, CANF_NONE);
		if (!done) 
			hal_can_cancel_tx();

		if (_send_message_len > 7)
		{
			buf.u8[0] = seg1 | ((_send_message_len <= 14) ? 0x80 : 0); // Mark end of message
			for(int i=0; i<7; i++) 
				buf.u8[1+i] = _send_message_buffer[seg1 * 7 + 7 + i];
			done = hal_can_send((CAN_EP) { .Id = id2 }, &buf, 8, CANF_NONE);
			if (!done) 
				hal_can_cancel_tx();
		}

		if((_send_message_cnt & 0x1) == 1)	// Discount on each 4th repeated sending
			_send_message_len -= 14;
		_send_message_cnt++;
		return 1;
	}

	return 0;	// A new large message can be delivered
}

static inline int _next_power_of_2(int n)
{
	n--;
	n |= n >> 1; 
	n |= n >> 2; 
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n+1;
}

static unsigned long _recv_message_marks = 0;
static unsigned char _recv_message_buffer[MULTIPACKET_MAX_LEN];

const unsigned char* multipacket_msg_receive (int* recv_len, const CAN_MSG* msg)
{
	*recv_len = 0;
	const unsigned char* tmsg = &msg->Data.u8[0];
	int packet_idx = tmsg[0] & 0x7f;
	int end_packet = tmsg[0] >> 7;
	if((packet_idx * 7) > (MULTIPACKET_MAX_LEN - 7))
		return 0;	// MULTIPACKET_MAX_LEN overflow!
	for(int i=0; i<7; i++)
		_recv_message_buffer[packet_idx * 7 + i] = tmsg[1 + i];
	if (packet_idx == 0)
		_recv_message_marks = 1;	// Init of message recording
	else
		_recv_message_marks |= 1 << (tmsg[0] & 0x7f);
	if (end_packet)	// End of large message mark
	{ 
		int msg_len = (packet_idx + 1) * 7;	// max size, rounded to mult. of 7
		int marks_needed = _next_power_of_2(msg_len) - 1;
		if ((marks_needed & _recv_message_marks) == marks_needed)
		{
			_recv_message_marks = 0;
			*recv_len = msg_len;
			return _recv_message_buffer;
		}
		else
		{
   			_recv_message_marks = -1;	// Msg incomplete, fail!
			return 0;
		}
	}
	return 0;
}

unsigned char* multipacket_msg_reset ( int len)
{
	if(_send_message_len > 0)
		return 0; // If previous msg not finished, fail
	if (len > MULTIPACKET_MAX_LEN)
		len = MULTIPACKET_MAX_LEN;
	_send_message_cnt = 0;
	_send_message_len = len;
	return _send_message_buffer;
}





