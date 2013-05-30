#include <net/tcp_io.h>
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>
#include "relay.h"
#include "rf.h"

#define SERVER_THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _thread_stack[SERVER_THREAD_STACK];

COMM_IO_ENTRY _comm0, _comm1;
TCP_IO_ENTRY _socket;
static unsigned char _buffer[1024] ;

#define TCP_BUFFER_SIZE 8192 // tiny window for stress
static unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static void *_server(void *);
static void _parse_input();

static RF_STATE _reader0, _reader1;
static void _parse_rf(RF_CARD *card, void *state);

void server_start()
{
	exos_thread_create(&_thread, 0, _thread_stack, SERVER_THREAD_STACK, NULL, _server, NULL); 
}

static COMM_IO_ENTRY *_open(COMM_IO_ENTRY *io, char *name)
{
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_node(NULL, name);
	if (dev_node != NULL && 
		dev_node->Type == EXOS_TREE_NODE_DEVICE)
	{
		comm_io_create(io, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT);
		if (comm_io_open(io) == 0)
		{
			int done = sprintf(_buffer, "'%s' ready!\r\n", dev_node->Name);
			done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
			return io;
		}
	}
	return 0;
}

static void *_server(void *arg)
{
	int err;
	int done;

	while(1)
	{
		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = _rcv_buffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = _snd_buffer, .SndBufferSize = TCP_BUFFER_SIZE };

		net_tcp_io_create(&_socket, EXOS_IOF_WAIT);

		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = 23 };
		err = net_io_bind((NET_IO_ENTRY *)&_socket, &local); 
		err = net_io_listen((NET_IO_ENTRY *)&_socket);

		err = net_io_accept((NET_IO_ENTRY *)&_socket, (NET_IO_ENTRY *)&_socket, &buffers);
		hal_led_set(0, 1);
		
		exos_io_set_timeout((EXOS_IO_ENTRY *)&_socket, 2000); // NOTE: we didn't set timeout before because we don't want accept()  to timeout

		COMM_IO_ENTRY *comm0;
		COMM_IO_ENTRY *comm1;
		int count = 0;
		EXOS_EVENT *events[3];
		events[count++] = &_socket.InputEvent;

		comm0 = _open(&_comm0, "dev/usbftdi0");
		if (comm0) events[count++] = &comm0->InputEvent;
		comm1 = _open(&_comm1, "dev/usbftdi1");
		if (comm1) events[count++] = &comm1->InputEvent;

		rf_init_state(&_reader0, _parse_rf, "R0");
		rf_init_state(&_reader1, _parse_rf, "R1");

#ifdef DEBUG
		done = sprintf(_buffer, "Connection accepted!\r\n");
		done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);				
#endif
				
		while(1)
		{
			done = exos_event_wait_multiple(events, count, EXOS_TIMEOUT_NEVER);

			if (_socket.InputEvent.State)
			{
				done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024);
				if (done < 0) break;

				if (done >= 1) _parse_input();
			}

			if (_comm0.InputEvent.State)
			{
				done = exos_io_read((EXOS_IO_ENTRY *)&_comm0, _buffer, 1024);
				if (done < 0) break;

				rf_parse(&_reader0, _buffer, done);
			}
			if (_comm1.InputEvent.State)
			{
				done = exos_io_read((EXOS_IO_ENTRY *)&_comm1, _buffer, 1024);
				if (done < 0) break;

				rf_parse(&_reader1, _buffer, done);
			}
		}

		if (comm0) comm_io_close(comm0);
		if (comm1) comm_io_close(comm1);
			
		hal_led_set(0, 0);
		net_io_close((NET_IO_ENTRY *)&_socket, &buffers);
	}
}

static void _parse_input()
{
	switch(_buffer[0])
	{
		case 'A':
			open_relay(0, 1<<0, 1000);
			break;
		case 'a':
			open_relay(0, 1<<1, 1000);
			break;
		case 'B':
			open_relay(1, 1<<0, 1000);
			break;
		case 'b':
			open_relay(1, 1<<1, 1000);
			break;
	}
}

static void _parse_rf(RF_CARD *card, void *state)
{
	char buf_out[32];
	int done = sprintf(buf_out, "%s:%x%x\r\n", state, (unsigned long)(card->Id >> 32), (unsigned long)card->Id);
	done = exos_io_write((EXOS_IO_ENTRY *)&_socket, buf_out, done);
}
