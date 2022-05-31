#include "server.h"
#ifndef RFGATE_UDP
#include <net/tcp_io.h>
#else
#include <net/udp_io.h>
#endif
#include <comm/comm.h>
#include <kernel/tree.h>
#include <support/board_hal.h>
#include <stdio.h>
#include <string.h>
#include "relay.h"
#include "rf.h"
#include "parser.h"

#define SERVER_THREAD_STACK 1024
static EXOS_THREAD _thread;
static unsigned char _thread_stack[SERVER_THREAD_STACK];

COMM_IO_ENTRY _comm0, _comm1;
static unsigned char _buffer[1024] ;
static unsigned char _buf_out[1024] __attribute__((section(".dma")));

#ifndef RFGATE_UDP
TCP_IO_ENTRY _socket;
#define TCP_BUFFER_SIZE 8192 // tiny window for stress
static unsigned char _rcv_buffer[TCP_BUFFER_SIZE]; 
static unsigned char _snd_buffer[TCP_BUFFER_SIZE] __attribute__((section(".dma")));
static unsigned char _line_buffer[256];
#else
IP_PORT_ADDR _remote;
UDP_IO_ENTRY _socket;
#endif
static void *_server(void *);

static RF_STATE _reader0, _reader1;
static void _rf_callback(RF_CARD *card, void *state);
static void _parse_input(unsigned char *buffer, int length);

void server_start()
{
	exos_thread_create(&_thread, 0, _thread_stack, SERVER_THREAD_STACK, NULL, _server, NULL); 
}

static COMM_IO_ENTRY *_open(COMM_IO_ENTRY *io, char *name)
{
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, name);
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
	return NULL;
}

static void *_server(void *arg)
{
	int err;
	int done;

	while(1)
	{
#ifndef RFGATE_UDP
		LINE_PARSER parser;
		parser_create(&parser, _line_buffer, sizeof(_line_buffer));

		EXOS_IO_STREAM_BUFFERS buffers = (EXOS_IO_STREAM_BUFFERS) {
			.RcvBuffer = _rcv_buffer, .RcvBufferSize = TCP_BUFFER_SIZE,
			.SndBuffer = _snd_buffer, .SndBufferSize = TCP_BUFFER_SIZE };

		net_tcp_io_create(&_socket, EXOS_IOF_WAIT);
#else
		net_udp_io_create(&_socket, EXOS_IOF_WAIT);
		_remote = (IP_PORT_ADDR) { .Address = IP_ADDR_BROADCAST, .Port = 5000 };
#endif

		IP_PORT_ADDR local = (IP_PORT_ADDR) { .Address = IP_ADDR_ANY, .Port = 23 };
		err = net_io_bind((NET_IO_ENTRY *)&_socket, &local); 

#ifndef RFGATE_UDP
		err = net_io_listen((NET_IO_ENTRY *)&_socket);
		err = net_io_accept((NET_IO_ENTRY *)&_socket, (NET_IO_ENTRY *)&_socket, &buffers);
#else
		exos_thread_sleep(1000);	// wait for usb devices to autoconfigure
#endif
		hal_led_set(0, 1);
		
		exos_io_set_timeout((EXOS_IO_ENTRY *)&_socket, 2000); // NOTE: we didn't set timeout before because we don't want accept() to timeout

		COMM_IO_ENTRY *comm0;
		COMM_IO_ENTRY *comm1;
		int count = 0;
		EXOS_EVENT *events[3];
		events[count++] = &_socket.InputEvent;

		comm0 = _open(&_comm0, "dev/usbftdi0");
		if (comm0) events[count++] = &comm0->InputEvent;
		comm1 = _open(&_comm1, "dev/usbftdi1");
		if (comm1) events[count++] = &comm1->InputEvent;

		rf_init_state(&_reader0, _rf_callback, "R0");
		rf_init_state(&_reader1, _rf_callback, "R1");

#if !defined RFGATE_UDP && defined DEBUG
		done = sprintf(_buffer, "Connection accepted!\r\n");
		done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buffer, done);
#endif
				
		while(1)
		{
			done = exos_event_wait_multiple(events, count, 10000);
			if (done < 0) 
				break; // activity timeout -> close conn

			if (_socket.InputEvent.State)
			{
#ifndef RFGATE_UDP
				done = exos_io_read((EXOS_IO_ENTRY *)&_socket, _buffer, 1024);
#else
				done = net_io_receive((NET_IO_ENTRY *)&_socket, _buffer, 1024, &_remote);
#endif
				if (done < 0) break;

				if (done >= 1)
				{
#ifndef RFGATE_UDP
					parser_parse_line(&parser, _parse_input, _buffer, done);
#else
					_parse_input(_buffer, done);
#endif
				}
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
#ifndef RFGATE_UDP
		net_io_close((NET_IO_ENTRY *)&_socket, &buffers);
#endif
	}
}

static void _rf_callback(RF_CARD *card, void *state)
{
	int done = sprintf(_buf_out, "%s:%x%x\r\n", state, (unsigned long)(card->Id >> 32), (unsigned long)card->Id);

#ifndef RFGATE_UDP
	done = exos_io_write((EXOS_IO_ENTRY *)&_socket, _buf_out, done);
#else
	_remote.Port = 5000;
	done = net_io_send((NET_IO_ENTRY *)&_socket, _buf_out, done, &_remote);
#endif
}

static void _parse_input(unsigned char *buffer, int length)
{
	unsigned int mask, time = 0;
	unsigned char buf2[16];
	int done;
	done = parse_string(&buffer, buf2, 15);
	if (done)
	{ 
		if (strcmp(buf2, "set") == 0)
		{
			done = parse_hex(&buffer, &mask);
			if (done)
			{
				done = parse_dec(&buffer, &time);
	
				relay_set(0, mask, mask, time);
			}
		}
		else if (strcmp(buf2, "reset") == 0)
		{
			done = parse_hex(&buffer, &mask);
			if (done)
			{
				done = parse_dec(&buffer, &time);
	
				relay_set(0, mask, 0, time);
			}			
		}
	}
}

