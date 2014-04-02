#include "relay.h"
#include <kernel/thread.h>
#include <support/misc/can_receiver.h>
#include <support/can_hal.h>
#include <support/board_hal.h>
#include <net/udp_io.h>
#include <stdio.h>

static CAN_HANDLER _handler;
static IP_PORT_ADDR _remote_ep = { .Address = {255, 255, 255, 255}, .Port = 5001 };
static unsigned char _buf_out[256];

#define THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _stack[THREAD_STACK];

static void *_thread_func(void *arg);

void relay_initialize()
{
	exos_thread_create(&_thread, 0, _stack, THREAD_STACK, NULL, _thread_func, NULL);
}

static void *_thread_func(void *arg)
{
	can_receiver_initialize();
	can_receiver_add_handler(&_handler, 0, 0x301, 0x301);

	hal_can_initialize(0, 250000, CAN_INITF_DISABLE_RETRANSMISSION);
	hal_can_initialize(1, 250000, CAN_INITF_DISABLE_RETRANSMISSION);

	int done;
	UDP_IO_ENTRY udp;
	net_udp_io_create(&udp, EXOS_IOF_NONE);
//	IP_PORT_ADDR local_ep = (IP_PORT_ADDR){ .Port = 501 };
//	done = net_io_bind((NET_IO_ENTRY *)&udp, &local_ep);

	CAN_MSG msg;
	while(1)
	{
		done = can_receiver_read(&_handler, &msg, 10000);
		if (done)
		{
//			const char *relay_name = msg.Data.u16[1] ? "T1" : "T0";
			done = sprintf(_buf_out, "T0:%x\r\n", /*relay_name,*/ (unsigned long)msg.Data.u16[0]);
			net_io_send((NET_IO_ENTRY *)&udp, _buf_out, done, &_remote_ep);
		}
	}
}

void relay_set(int unit, unsigned short mask, unsigned short value, unsigned long time)
{
	CAN_BUFFER data;
	data.u16[0] = mask;
	data.u16[1] = value;
	data.u32[1] = time;
	hal_can_send((CAN_EP) { .Id = 0x200, .Bus = unit }, &data, 8, CANF_PRI_ANY);
}
