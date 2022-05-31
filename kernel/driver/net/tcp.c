// IP Stack. Transmission Control Protocol Support
// by Miguel Fides

#include "tcp.h"
#include "udp.h"
#include "tcp_io.h"
#include "tcp_service.h"
#include <kernel/panic.h>

static inline void _handle_input(TCP_IO_ENTRY *io, TCP_HEADER *tcp, void *data, unsigned short data_length);

int net_tcp_input(NET_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip)
{
	unsigned short msg_length;
	TCP_HEADER *tcp = (TCP_HEADER *)net_ip_get_payload(ip, &msg_length);

	NET_MBUF msg;
	net_mbuf_init(&msg, tcp, 0, msg_length);
	unsigned short checksum = net_tcp_checksum(&ip->SourceIP, &ip->DestinationIP, &msg, 0);
	if (0 == checksum)
	{
		// find handler for destination port
		unsigned short local_port = NTOH16(tcp->DestinationPort);
		unsigned short src_port = NTOH16(tcp->SourcePort);

		TCP_IO_ENTRY *io;
		if (NULL != (io = __tcp_io_find_io(local_port, ip->SourceIP, src_port)) ||
			NULL != (io = __tcp_io_find_io(local_port, IP_ADDR_ANY, 0)))
		{
			exos_mutex_lock(&io->Mutex);
			void *data = (void *)tcp + (tcp->DataOffset << 2);
			unsigned short data_length = msg_length - (tcp->DataOffset << 2);

			switch(io->State)
			{
				case TCP_STATE_SYN_SENT:
					if (tcp->Flags.SYN && tcp->Flags.ACK && NTOH32(tcp->Ack) == io->SndSeq + 1)
					{
						io->RcvNext = NTOH32(tcp->Sequence) + 1;
						io->SndSeq++;
						io->SndFlags = (TCP_FLAGS) { .ACK = 1 };
						io->State = TCP_STATE_ESTABLISHED;
						net_tcp_service(io, 0);

						exos_event_set(&io->OutputEvent);
					}
					break;
				case TCP_STATE_LISTEN:
					if (tcp->Flags.SYN && !tcp->Flags.ACK)
					{
						TCP_INCOMING_CONN* conn = __tcp_get_incoming_conn();
						if (conn != NULL)
						{
							*conn = (TCP_INCOMING_CONN) {
								.Adapter = adapter,
								.RemoteEP.IP = ip->SourceIP, 
								.RemotePort = NTOH16(tcp->SourcePort),
								.LocalPort = io->LocalPort,
								.Sequence = NTOH32(tcp->Sequence) };
							
							if (!net_ip_resolve(adapter, &conn->RemoteEP))
								kernel_panic(KERNEL_ERROR_UNKNOWN);

							exos_fifo_queue(&io->AcceptQueue, (EXOS_NODE *)conn);
						}
						else
						{
							// TODO: cannot queue incoming connection
						}
					}
					break;
				case TCP_STATE_SYN_RECEIVED:
					if (!tcp->Flags.SYN && tcp->Flags.ACK && NTOH32(tcp->Sequence) == io->RcvNext)
					{
						io->SndSeq = NTOH32(tcp->Ack);
						io->SndFlags = (TCP_FLAGS) { };
						io->State = TCP_STATE_ESTABLISHED;
						
						exos_event_set(&io->OutputEvent);
					}
					break;
				case TCP_STATE_ESTABLISHED:
					_handle_input(io, tcp, data, data_length);
					break;
				case TCP_STATE_FIN_WAIT_1:
					if (NTOH32(tcp->Sequence) == io->RcvNext)
					{
						if (tcp->Flags.ACK)
						{
							io->SndSeq = NTOH32(tcp->Ack);
							if (tcp->Flags.FIN)
							{
								io->RcvNext++;
								io->SndFlags = (TCP_FLAGS) { .ACK = 1 };
								io->State = TCP_STATE_CLOSING;
							}
							else
							{
								io->SndFlags = (TCP_FLAGS) { .ACK = 0 };
								io->State = TCP_STATE_FIN_WAIT_2;
							}
						}
						io->RcvNext += data_length;
						net_tcp_service(io, 0);
					}
					break;
				case TCP_STATE_FIN_WAIT_2:
					if (NTOH32(tcp->Sequence) == io->RcvNext)
					{
						if (tcp->Flags.FIN)
						{
							io->RcvNext++;
							io->SndFlags = (TCP_FLAGS) { .ACK = 1 };
							io->State = TCP_STATE_TIME_WAIT;
						}
						io->RcvNext += data_length;
						net_tcp_service(io, 0);
					}
					break;
				case TCP_STATE_CLOSING:
					if (tcp->Flags.ACK && NTOH32(tcp->Sequence) == io->RcvNext)
					{
						io->SndSeq = NTOH32(tcp->Ack);
						io->State = TCP_STATE_TIME_WAIT;
						net_tcp_service(io, 10);
					}
					break;
				case TCP_STATE_LAST_ACK:
					if (tcp->Flags.ACK && NTOH32(tcp->Sequence) == io->RcvNext)
					{
						io->SndSeq = NTOH32(tcp->Ack);
						io->State = TCP_STATE_CLOSED;
						net_tcp_service(io, 10);
					}
					break;
			}
   			exos_mutex_unlock(&io->Mutex);
		}
	}
	return 0;
}

static void _handle_input(TCP_IO_ENTRY *io, TCP_HEADER *tcp, void *data, unsigned short data_length)
{
	unsigned long seq = NTOH32(tcp->Sequence);
	unsigned long ack = NTOH32(tcp->Ack);

	if (seq == io->RcvNext)
	{
		// handle ACK
		if (tcp->Flags.ACK)
		{
			int offset = (ack - io->SndSeq);
			if (offset > 0)
			{
				io->SndSeq += exos_io_buffer_discard(&io->SndBuffer, offset);

				int rem = exos_io_buffer_avail(&io->SndBuffer);
				if (rem == 0)
				{
					io->SndFlags.PSH = 0;
				}
			}
		}

		// handle incoming
		if (tcp->Flags.RST)
		{
			io->State = TCP_STATE_CLOSED;
            net_tcp_service(io, 0);
		}
		else
		{
			int service_delay = -1;
			if (data_length != 0)
			{
				int done = exos_io_buffer_write(&io->RcvBuffer, data, data_length);
				io->RcvNext += done;
				io->SndFlags.ACK = 1;
				service_delay = 10;
				// NOTE: this time is to allow our application to send some (reply) data and save an empty (no-data) ack
			}
			else
			{
				io->SndFlags.ACK = 0;
			}
			
			if (tcp->Flags.FIN)
			{
				io->RcvNext++;
				io->SndFlags.ACK = 1;
				io->State = TCP_STATE_CLOSE_WAIT;
				service_delay = 0;
			}

			if (service_delay >= 0)
				net_tcp_service(io, service_delay);	// FIXME: make time clearance configurable
		}
	}
}

unsigned short net_tcp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset)
{
	int total_length = 0;
	unsigned long sum = 0;
	NET_MBUF local;

	if (mbuf != NULL && offset != 0)
	{
		mbuf = net_mbuf_seek(&local, mbuf, offset) ? &local : NULL;
	}

	offset = 0;
	while (mbuf != NULL)
	{
		int length = mbuf->Length - (mbuf->Offset + offset);
		NET16_T *data = (NET16_T *)(mbuf->Buffer + mbuf->Offset + offset);

		for(int i = 0; i < (length >> 1); i++)
			sum += NTOH16(data[i]);

		total_length += length;
		mbuf = mbuf->Next;
			
		if (length & 1)
		{
			unsigned short padded = ((unsigned char *)data)[length - 1] << 8;
			if (mbuf != NULL) 
			{
				padded |= *(unsigned char *)(mbuf->Buffer + mbuf->Offset);
      			total_length++;
			}
			sum += padded;
			offset = 1;
		}
		else offset = 0;
	}

	IP_PSEUDO_HEADER pseudo = { *source_ip, *dest_ip, 0, IP_PROTOCOL_TCP, HTON16(total_length) };
	NET16_T *pseudo_words = (NET16_T *)&pseudo;
	for(int i = 0; i < (sizeof(IP_PSEUDO_HEADER) >> 1); i++)
	{
		sum += NTOH16(pseudo_words[i]);
	}
    // add carry to do 1's complement sum
	sum = (sum & 0xFFFF) + (sum >> 16);
	sum += (sum >> 16);	
	return (unsigned short)~sum;
}


