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

void net_tcp_service_start()
{
	list_initialize(&_entries);
	exos_mutex_create(&_entries_mutex);
	
	exos_fifo_create(&_free_incoming_connections, NULL);
	for (int i = 0; i < TCP_MAX_PENDING_CONNECTIONS; i++)
	{
		exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)&_connections[i]);
	}

	exos_thread_create(&_thread, 1, _thread_stack, TCP_SERVICE_THREAD_STACK, _service, NULL);
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

void __tcp_io_remove_io(TCP_IO_ENTRY *io)
{
	exos_mutex_lock(&_entries_mutex);
#ifdef DEBUG
	if (!list_find_node(&_entries, (EXOS_NODE *)io))
		kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
	list_remove((EXOS_NODE *)io);
	exos_mutex_unlock(&_entries_mutex);
}

TCP_INCOMING_CONN *__tcp_get_incoming_conn()
{
	return (TCP_INCOMING_CONN *)exos_fifo_dequeue(&_free_incoming_connections);
}

static int _bind(TCP_IO_ENTRY *io, unsigned short local_port, IP_ADDR remote_ip, unsigned short remote_port)
{
	if (local_port == 0)
		return 0;	// FIXME: auto allocate port number?
	
	int done = 0;
	exos_mutex_lock(&_entries_mutex);
	TCP_IO_ENTRY *existing = __tcp_io_find_io(local_port, remote_ip, remote_port);
	if (existing == NULL)
	{
		io->LocalPort = local_port;
		io->State = TCP_STATE_CLOSED;
		list_add_tail(&_entries, (EXOS_NODE *)io);
		done = 1;
	}
	exos_mutex_unlock(&_entries_mutex);
	return done;
}

int net_tcp_bind(TCP_IO_ENTRY *io, unsigned short local_port)
{
	return _bind(io, local_port, IP_ADDR_ANY, 0);
}

int net_tcp_accept(TCP_IO_ENTRY *io, EXOS_IO_STREAM_BUFFERS *buffers, TCP_INCOMING_CONN *conn)
{
	if (_bind(io, conn->LocalPort, conn->RemoteEP.IP, conn->RemotePort))
	{
		exos_io_buffer_create(&io->RcvBuffer, buffers->RcvBuffer, buffers->RcvBufferSize);
		io->RcvBuffer.NotEmptyEvent = &io->InputEvent;
	
		exos_io_buffer_create(&io->SndBuffer, buffers->SndBuffer, buffers->SndBufferSize);
		io->SndBuffer.NotFullEvent = &io->OutputEvent;

		io->Adapter = conn->Adapter;
		io->RemotePort = conn->RemotePort;
		io->RemoteEP = conn->RemoteEP;

		io->SndAck = io->SndNext = 0; // FIXME: use random for security

		io->RcvNext = conn->Sequence + 1;
		io->SndFlags = (TCP_FLAGS) { .SYN = 1, .ACK = 1 };
		__tcp_send(io);
		io->SndNext++;

		io->State = TCP_STATE_SYN_RECEIVED;
	}

	exos_fifo_queue(&_free_incoming_connections, (EXOS_NODE *)conn);
	return 1;
}

static int _send(TCP_IO_ENTRY *io)
{
	EXOS_EVENT completed_event;
	exos_event_create(&completed_event);
	ETH_OUTPUT_BUFFER resp = (ETH_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };

	TCP_HEADER *tcp = net_ip_output(io->Adapter, &resp, sizeof(TCP_HEADER), &io->RemoteEP, IP_PROTOCOL_TCP);
	if (tcp != NULL)
	{
		tcp->SourcePort = HTON16(io->LocalPort);
		tcp->DestinationPort = HTON16(io->RemotePort);
		tcp->Sequence = HTON32(io->SndNext);
		
		tcp->Ack = HTON32(io->RcvNext);
		tcp->DataOffset = sizeof(TCP_HEADER) >> 2;
		tcp->Reserved = 0;
		tcp->Flags = io->SndFlags;
		unsigned short rcv_free = (io->RcvBuffer.Size - 1) 
			- exos_io_buffer_avail(&io->RcvBuffer);
		tcp->WindowSize = HTON16(rcv_free);
		tcp->Checksum = HTON16(0);
		tcp->UrgentPtr = HTON16(0);

		NET_MBUF mbuf1;
		NET_MBUF mbuf2;
		int offset = (io->SndNext - io->SndBase);
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
		int tcp_length = sizeof(TCP_HEADER) + payload;

		int tcp_offset = (unsigned char *)tcp - (unsigned char *)resp.Buffer.Buffer; 
		unsigned short checksum = net_tcp_checksum(&io->Adapter->IP, &io->RemoteEP.IP, &resp.Buffer, tcp_offset);
		tcp->Checksum = HTON16(checksum);
		int done = net_ip_send_output(io->Adapter, &resp, tcp_length);
		
		if (done)
			exos_event_wait(&completed_event, EXOS_TIMEOUT_NEVER);

		return payload;
	}
	return -1;
}

static void _handle(TCP_IO_ENTRY *io)
{
	exos_mutex_lock(&io->Mutex);

	if (io->State == TCP_STATE_ESTABLISHED)
	{
		int remaining = exos_io_buffer_avail(&io->SndBuffer);
		remaining -= (io->SndNext - io->SndBase);

		if (io->SndFlags.PSH || 
			io->SndFlags.ACK ||
			remaining >= 1) // FIXME: use equiv. to SO_SNDLOWAT 
		{
			io->SndFlags.ACK = 1;
			int msg_length = _send(io);
			io->SndNext += msg_length;

			io->SndFlags.ACK = 0;
			if (msg_length == remaining) io->SndFlags.PSH = 0;
		}
	}
	else if (io->State == TCP_STATE_CLOSE_WAIT)
	{
		io->SndFlags.FIN = 1;
		 _send(io);
		io->SndNext++;

		io->State = TCP_STATE_LAST_ACK;
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

					if (io->State == TCP_STATE_CLOSED)
					{
						node = node->Pred;
						list_remove(node);
						// FIXME: use a "removed" state?
					}
					rem = io->ServiceWait; 
				}
				if (rem < rem_min)
				{
					rem_min = rem;
				}
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

int __tcp_send(TCP_IO_ENTRY *io)
{
	return _send(io);
}


