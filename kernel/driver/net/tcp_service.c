#include "tcp_service.h"
#include "tcp_io.h"
#include <kernel/timer.h>
#include <kernel/panic.h>

static EXOS_LIST _entries;	// bound io entries
static EXOS_MUTEX _entries_mutex;
static EXOS_THREAD _thread;
static unsigned char _thread_stack[TCP_SERVICE_THREAD_STACK];
static int _wakeup_signal;

static EXOS_FIFO _free_incoming_connections;
static TCP_INCOMING_CONN _connections[TCP_MAX_PENDING_CONNECTIONS];

static void *_service(void *arg);
static int _send(TCP_IO_ENTRY *io);

void net_tcp_service_start()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
	
	exos_fifo_create(&_free_incoming_connections, NULL);
	for (int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++)
	{
		exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)&_connections[i]);
	}

	exos_thread_create(&_thread, 1, _thread_stack, TCP_SERVICE_THREAD_STACK, NULL, _service, NULL);
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
			found = io;
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
	return 1;
}

int net_tcp_accept(TCP_IO_ENTRY *io, const EXOS_IO_STREAM_BUFFERS *buffers, TCP_INCOMING_CONN *conn)
{
	if (conn == NULL || buffers == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&io->Mutex);

	exos_io_buffer_create(&io->RcvBuffer, buffers->RcvBuffer, buffers->RcvBufferSize);
	io->RcvBuffer.NotEmptyEvent = &io->InputEvent;

	exos_io_buffer_create(&io->SndBuffer, buffers->SndBuffer, buffers->SndBufferSize);
	io->SndBuffer.NotFullEvent = &io->OutputEvent;

	io->Adapter = conn->Adapter;
	io->LocalPort = conn->LocalPort;
	io->RemotePort = conn->RemotePort;
	io->RemoteEP = conn->RemoteEP;

	io->RcvNext = conn->Sequence + 1;
	io->SndAck = io->SndNext = 0; // FIXME: use random for security

	io->SndFlags = (TCP_FLAGS) { .SYN = 1, .ACK = 1 };
	int done = _bind(io, TCP_STATE_SYN_RECEIVED);

	exos_mutex_unlock(&io->Mutex);

	// recycle conn
	exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)conn);

	if (done) 
	{
		net_tcp_service(io, 0);
        exos_event_wait(&io->OutputEvent, EXOS_TIMEOUT_NEVER);	// FIXME: allow timeout
	}
	return done;
}

int net_tcp_close(TCP_IO_ENTRY *io)
{
	EXOS_EVENT event;
	exos_event_create(&event);
	io->CloseEvent = &event;

	exos_mutex_lock(&io->Mutex);
	switch(io->State)
	{
		case TCP_STATE_CLOSE_WAIT:
			io->SndFlags.FIN = 1;
			io->State = TCP_STATE_LAST_ACK;
			break;
		case TCP_STATE_SYN_RECEIVED:
		case TCP_STATE_ESTABLISHED:
			io->SndFlags.FIN = 1;
			io->State = TCP_STATE_FIN_WAIT_1;
			break;
		case TCP_STATE_LISTEN:
			io->State = TCP_STATE_CLOSED;
			break;
	}
	exos_mutex_unlock(&io->Mutex);

	net_tcp_service(io, 0);

	exos_event_wait(io->CloseEvent, EXOS_TIMEOUT_NEVER);

#ifdef DEBUG
	if (io->State != TCP_STATE_CLOSED)
		kernel_panic(KERNEL_ERROR_UNKNOWN);
#endif
	return 1;
}

static void _handle(TCP_IO_ENTRY *io)
{
	exos_mutex_lock(&io->Mutex);

	int remaining = 0;
	switch(io->State)
	{
		case TCP_STATE_ESTABLISHED:
			remaining = exos_io_buffer_avail(&io->SndBuffer);
			remaining -= (io->SndNext - io->SndAck);
       		if (remaining >= 1) // FIXME: use equiv. to SO_SNDLOWAT 
				io->SndFlags.ACK = 1;
			break;
		case TCP_STATE_CLOSE_WAIT:
			exos_event_set(&io->InputEvent);
			exos_event_set(&io->OutputEvent);
			break;
		case TCP_STATE_TIME_WAIT:
			// NOTE: do nothing
			io->State = TCP_STATE_CLOSED;
			break;
	}

	if (io->SndFlags.PSH || io->SndFlags.SYN || io->SndFlags.FIN ||
		io->SndFlags.ACK) 
	{
		io->SndFlags.ACK = 1;
		int done = _send(io);
		if (done >= 0)
		{
			io->SndNext += done;
			io->SndFlags.ACK = 0;
            io->SndFlags.SYN = 0;
			io->SndFlags.FIN = 0;
			if (done == remaining) io->SndFlags.PSH = 0;
		}
	}
	exos_mutex_unlock(&io->Mutex);
}


static void *_service(void *arg)
{
	_wakeup_signal = exos_signal_alloc();

	while(1)
	{
		exos_mutex_lock(&_entries_mutex);
		
		unsigned long rem_min = 0x7fffffff;
		FOREACH(node, &_entries)
		{
			TCP_IO_ENTRY *io = (TCP_IO_ENTRY *)node;
			if (io->State != TCP_STATE_CLOSED)
			{
				int rem = io->ServiceWait - exos_timer_elapsed(io->ServiceTime);
				if (rem <= 0)
				{
					io->ServiceTime = exos_timer_time();
					io->ServiceWait = 0x7fffffff;

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

				exos_event_set(io->CloseEvent);
			}
		}
		exos_mutex_unlock(&_entries_mutex);

		if (rem_min > 0)
		{
			exos_signal_wait(1 << _wakeup_signal, rem_min);
		}
	}
}

void net_tcp_service(TCP_IO_ENTRY *io, int wait)
{
	io->ServiceTime = exos_timer_time();
	io->ServiceWait = wait;
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
		int offset = (io->SndNext - io->SndAck);
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
					if (payload2 > max_payload)
						payload2 = max_payload;

					net_mbuf_init(&mbuf2, buffer, 0, payload2);
					net_mbuf_append(&resp.Buffer, &mbuf2);
					payload += payload2;
				}
			}
		}

		tcp->Sequence = HTON32(io->SndNext);
		
		tcp->Ack = HTON32(io->RcvNext);
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
	return -1;
}



