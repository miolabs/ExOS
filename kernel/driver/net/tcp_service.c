#include "tcp_service.h"
#include "tcp_io.h"
#include "ip.h"
#include <kernel/timer.h>
#include <kernel/panic.h>

static EXOS_LIST _entries;	// bound io entries
static EXOS_MUTEX _entries_mutex;

#ifndef TCP_MAX_PENDING_CONNECTIONS
#define TCP_MAX_PENDING_CONNECTIONS 4
#endif

#define TCP_SERVICE_THREAD_STACK 512
static EXOS_THREAD _thread;
static unsigned char _thread_stack[TCP_SERVICE_THREAD_STACK];
static int _wakeup_signal;

static EXOS_FIFO _free_incoming_connections;
static TCP_INCOMING_CONN _connections[TCP_MAX_PENDING_CONNECTIONS];

static void *_service(void *arg);

void net_tcp_service_start()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
	
	exos_fifo_create(&_free_incoming_connections, NULL);
	for (int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++)
	{
		exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)&_connections[i]);
	}

	exos_thread_create(&_thread, 5, _thread_stack, TCP_SERVICE_THREAD_STACK, NULL, _service, NULL);
}

TCP_IO_ENTRY *__tcp_io_find_io(unsigned short local_port, IP_ADDR remote_ip, unsigned short remote_port)
{
	TCP_IO_ENTRY *found = NULL;
	exos_mutex_lock(&_entries_mutex);
	FOREACH(node, &_entries)
	{
		TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)node;
		if (io->LocalPort == local_port &&
			io->RemoteEP.IP.Value == remote_ip.Value && 
			io->RemotePort == remote_port)
		{
			found = io;
			break;
		}
	}
	exos_mutex_unlock(&_entries_mutex);
	return found;
}

TCP_INCOMING_CONN *__tcp_get_incoming_conn()
{
	return (TCP_INCOMING_CONN *)exos_fifo_dequeue(&_free_incoming_connections);
}

static int _bind(TCP_IO_ENTRY *io, TCP_STATE init_state)
{
	int done = 0;
	exos_mutex_lock(&_entries_mutex);
	TCP_IO_ENTRY *existing = __tcp_io_find_io(io->LocalPort, io->RemoteEP.IP, io->RemotePort);
	if (existing == NULL || existing == io)
	{
		io->State = init_state;
		if (existing == NULL) list_add_tail(&_entries, (EXOS_NODE *)io);
		done = 1;
	}
	exos_event_reset(&io->CloseEvent);
	exos_mutex_unlock(&_entries_mutex);
	return done;
}

int net_tcp_listen(TCP_IO_ENTRY *io, unsigned short local_port)
{
	exos_mutex_lock(&io->Mutex);

	int done = 0;
	if (io->State == TCP_STATE_CLOSED)
	{
		io->Adapter = NULL;
		io->LocalPort = local_port;
		io->RemotePort = 0;
		io->RemoteEP = (IP_ENDPOINT) { .IP = IP_ADDR_ANY };

		exos_fifo_create(&io->AcceptQueue, &io->InputEvent);

		if (_bind(io, TCP_STATE_LISTEN))
			done = 1;
	}

	exos_mutex_unlock(&io->Mutex);
	return done;
}

int net_tcp_accept(TCP_IO_ENTRY *io, const EXOS_IO_STREAM_BUFFERS *buffers, TCP_INCOMING_CONN *conn)
{
	if (conn == NULL || buffers == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&io->Mutex);

	int done = 0;
	if (io->State == TCP_STATE_CLOSED)
	{
		exos_io_buffer_create(&io->RcvBuffer, buffers->RcvBuffer, buffers->RcvBufferSize);
		io->RcvBuffer.NotEmptyEvent = &io->InputEvent;
	
		exos_io_buffer_create(&io->SndBuffer, buffers->SndBuffer, buffers->SndBufferSize);
		io->SndBuffer.NotFullEvent = &io->OutputEvent;
	
		io->Adapter = conn->Adapter;
		io->LocalPort = conn->LocalPort;
		io->RemotePort = conn->RemotePort;
		io->RemoteEP = conn->RemoteEP;
	
		io->RcvNext = conn->Sequence + 1;
		io->SndSeq = 0; // FIXME: use random for security
	
		io->SndFlags = (TCP_FLAGS) { .SYN = 1, .ACK = 1 };
		done = _bind(io, TCP_STATE_SYN_RECEIVED);

		// recycle conn
		exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)conn);
	}
	
	exos_mutex_unlock(&io->Mutex);

	if (done) 
	{
		net_tcp_service(io, 0);
		exos_event_wait(&io->OutputEvent, EXOS_TIMEOUT_NEVER);	// FIXME: allow timeout
	}
	return done;
}

static unsigned short _port_hack = 0;	// FIXME: this counter cycles port numbers

int net_tcp_connect(TCP_IO_ENTRY *io, const EXOS_IO_STREAM_BUFFERS *buffers, IP_PORT_ADDR *remote)
{
	if (buffers == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&io->Mutex);

	int done = 0;
	if (io->State == TCP_STATE_CLOSED)
	{
		exos_event_reset(&io->CloseEvent);

		_port_hack = (_port_hack + 1) % 1000;	// FIXME: implement a port allocation scheme

		io->Adapter = NULL;
		io->LocalPort = 10000 + _port_hack; // TODO: allocate high port number
		io->RemotePort = remote->Port;
		io->RemoteEP = (IP_ENDPOINT) { .IP = remote->Address };
		if (net_ip_get_adapter_and_resolve(&io->Adapter, &io->RemoteEP))
		{
			exos_io_buffer_create(&io->RcvBuffer, buffers->RcvBuffer, buffers->RcvBufferSize);
			io->RcvBuffer.NotEmptyEvent = &io->InputEvent;
		
			exos_io_buffer_create(&io->SndBuffer, buffers->SndBuffer, buffers->SndBufferSize);
			io->SndBuffer.NotFullEvent = &io->OutputEvent;
			
			io->RcvNext = 0; // FIXME: use random for security
			io->SndSeq = 0; // FIXME: use random for security
	
			io->SndFlags = (TCP_FLAGS) { .SYN = 1 };
			done = _bind(io, TCP_STATE_SYN_SENT);
		}
	}

	exos_mutex_unlock(&io->Mutex);
	
	if (done) 
	{
		net_tcp_service(io, 0);
		exos_event_wait(&io->OutputEvent, EXOS_TIMEOUT_NEVER);

		done = (io->State == TCP_STATE_ESTABLISHED);
	}

	if (!done)
	{
		// destroy buffers
		io->RcvBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL };
		io->SndBuffer = (EXOS_IO_BUFFER) { .Buffer = NULL };	

		exos_event_set(&io->CloseEvent);
	}
	return done;
}

int net_tcp_close(TCP_IO_ENTRY *io)
{
	TCP_INCOMING_CONN *conn;
	int run_service = 0;
	exos_mutex_lock(&io->Mutex);
	if (io->State == TCP_STATE_LISTEN)
	{
		while(NULL != (conn = (TCP_INCOMING_CONN *)exos_fifo_dequeue(&io->AcceptQueue)))
		{
			exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)conn);
		}
		exos_mutex_lock(&_entries_mutex);
		list_remove((EXOS_NODE *)io);
		io->State = TCP_STATE_CLOSED;
		exos_event_set(&io->CloseEvent);
		exos_mutex_unlock(&_entries_mutex);
	}
	else if (io->State != TCP_STATE_CLOSED)
	{
		switch(io->State)
		{
			case TCP_STATE_CLOSE_WAIT:
				io->SndFlags = (TCP_FLAGS) { .FIN = 1, .ACK = 1 };
				io->State = TCP_STATE_LAST_ACK;
				break;
			case TCP_STATE_SYN_RECEIVED:
			case TCP_STATE_ESTABLISHED:
				io->SndFlags = (TCP_FLAGS) { .FIN = 1, .ACK = 1 };
				io->State = TCP_STATE_FIN_WAIT_1;
				break;
		}
		run_service = 1;
	}
	exos_mutex_unlock(&io->Mutex);

	if (run_service) 
		net_tcp_service(io, 0);

	exos_event_wait(&io->CloseEvent, EXOS_TIMEOUT_NEVER);

#ifdef DEBUG
	if (io->State != TCP_STATE_CLOSED)
		kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
	return 1;
}

void net_tcp_service(TCP_IO_ENTRY *io, int wait)
{
	io->ServiceTime = exos_timer_time();
	io->ServiceWait = wait;
	io->ServiceRetry = 0;
	exos_signal_set(&_thread, 1 << _wakeup_signal);
}

static int _send(TCP_IO_ENTRY *io)
{
	EXOS_EVENT completed_event;
	exos_event_create(&completed_event);
	NET_OUTPUT_BUFFER resp = (NET_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };

	TCP_HEADER *tcp = net_ip_output(io->Adapter, &resp, sizeof(TCP_HEADER), &io->RemoteEP, IP_PROTOCOL_TCP);
	if (tcp != NULL)
	{
		tcp->SourcePort = HTON16(io->LocalPort);
		tcp->DestinationPort = HTON16(io->RemotePort);

		NET_MBUF mbuf1;
		NET_MBUF mbuf2;
		int offset = 0; //FIXME: for send ahead (io->SndNext - io->SndAck);
		void *buffer;
		int payload = exos_io_buffer_peek(&io->SndBuffer, offset, &buffer);
		if (payload != 0)
		{
			int max_payload = ETH_MAX_PAYLOAD - ((void *)tcp - resp.Buffer.Buffer);

			if (payload > max_payload)
			{
				payload = max_payload;
				net_mbuf_init(&mbuf1, buffer, 0, payload);
				net_mbuf_append(&resp.Buffer, &mbuf1);
			}
			else
			{
				net_mbuf_init(&mbuf1, buffer, 0, payload);
				net_mbuf_append(&resp.Buffer, &mbuf1);

				int payload2 = exos_io_buffer_peek(&io->SndBuffer, offset + payload, &buffer);
				if (payload2 != 0)
				{
					payload += payload2;
					if (payload > max_payload)
						payload2 -= (payload - max_payload);

					net_mbuf_init(&mbuf2, buffer, 0, payload2);
					net_mbuf_append(&resp.Buffer, &mbuf2);
				}
			}
		}

		tcp->Sequence = HTON32(io->SndSeq);

		tcp->Ack = io->SndFlags.ACK ? HTON32(io->RcvNext) : HTON32(0);
		tcp->DataOffset = sizeof(TCP_HEADER) >> 2;
		tcp->Reserved = 0;
		tcp->Flags = io->SndFlags;

		unsigned short rcv_free = (io->RcvBuffer.Size - 1) - exos_io_buffer_avail(&io->RcvBuffer);
		tcp->WindowSize = HTON16(rcv_free);
		tcp->Checksum = HTON16(0);
		tcp->UrgentPtr = HTON16(0);

		int tcp_length = sizeof(TCP_HEADER) + payload;
		int tcp_offset = (unsigned char *)tcp - (unsigned char *)resp.Buffer.Buffer; 
		unsigned short checksum = net_tcp_checksum(&io->Adapter->IP, &io->RemoteEP.IP, &resp.Buffer, tcp_offset);
		tcp->Checksum = HTON16(checksum);
		int done = net_ip_send_output(io->Adapter, &resp, tcp_length);
		
		if (done)
			exos_event_wait(&completed_event, EXOS_TIMEOUT_NEVER);

		if (io->SndFlags.SYN || io->SndFlags.FIN) payload++;
		return payload;
	}

#ifdef DEBUG
	// TODO: take action on dropped output message
#endif
	return -1;
}

static void _handle(TCP_IO_ENTRY *io)
{
	exos_mutex_lock(&io->Mutex);

	switch(io->State)
	{
		case TCP_STATE_SYN_SENT:
			if (io->ServiceRetry < 3)
				io->ServiceWait = 1000;
			else io->State = TCP_STATE_CLOSED;
			break;
		case TCP_STATE_ESTABLISHED:
			if (io->SndFlags.PSH)
			{
				io->SndFlags.ACK = 1;
				io->ServiceWait = 100; // FIXME: use adaptive retransmit time
			}
			else io->ServiceWait = 30000; // FIXME: use configurable keep-alive time
			break;
		case TCP_STATE_CLOSE_WAIT:
			io->ServiceWait = 1000;
			exos_event_set(&io->InputEvent);
			exos_event_set(&io->OutputEvent);
			break;
		case TCP_STATE_TIME_WAIT:
			if (io->SndFlags.ACK && io->ServiceRetry == 0)	
			{
				// NOTE: send pending ack when coming from FIN_WAIT_2
				io->ServiceWait = 2000;	// FIXME: should be 2MSL
			}
			else io->State = TCP_STATE_CLOSED;
			break;
		case TCP_STATE_LAST_ACK:
			if (io->ServiceRetry < 3)
				io->ServiceWait = 1000;
			else io->State = TCP_STATE_CLOSED;
			break;
		case TCP_STATE_FIN_WAIT_1:
		case TCP_STATE_FIN_WAIT_2:
			if (io->ServiceRetry < 1) 
				io->ServiceWait = 5000;
			else io->State = TCP_STATE_CLOSED;
		default:
			io->ServiceWait = 30000;
			break;
	}

	if (io->State != TCP_STATE_CLOSED)
	{
		io->ServiceTime = exos_timer_time();
		
		if (io->SndFlags.PSH || io->SndFlags.SYN || io->SndFlags.FIN || 
			io->SndFlags.ACK)
		{
			int done = _send(io);
		}
	}

	io->ServiceRetry++;
	exos_mutex_unlock(&io->Mutex);
}

static void *_service(void *arg)
{
	_wakeup_signal = exos_signal_alloc();

	while(1)
	{
		exos_mutex_lock(&_entries_mutex);
		
		unsigned long rem_min = 0x7fffffff;	// max signed int
		FOREACH(node, &_entries)
		{
			TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)node;
			if (io->State == TCP_STATE_LISTEN)
				continue;

			if (io->State != TCP_STATE_CLOSED)
			{
				int rem = (unsigned long)io->ServiceWait - exos_timer_elapsed(io->ServiceTime);
				if (rem <= 0)
				{
					_handle(io);
					rem = io->ServiceWait; 
				}
				if (rem < rem_min)
				{
					rem_min = rem;
				}
			}
			if (io->State == TCP_STATE_CLOSED)
			{
				node = node->Pred;
				list_remove(node->Succ);

				exos_event_set(&io->OutputEvent);
				exos_event_set(&io->CloseEvent);
			}
		}
		exos_mutex_unlock(&_entries_mutex);

		if (rem_min > 0)
		{
			exos_signal_wait(1 << _wakeup_signal, rem_min);
		}
	}
}




