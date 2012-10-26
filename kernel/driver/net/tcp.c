// IP Stack. Transmission Control Protocol Support
// by Miguel Fides

#include "tcp.h"
#include "udp.h"
#include "tcp_io.h"
#include "tcp_service.h"

static inline void _handle_input(TCP_IO_ENTRY *io, TCP_HEADER *tcp, void *data, unsigned short data_length);

int net_tcp_input(ETH_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip)
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
		TCP_IO_ENTRY *io = __tcp_io_find_io(adapter, local_port, ip->SourceIP, src_port);
		if (io != NULL)
		{
			exos_mutex_lock(&io->Mutex);
			void *data = (void *)tcp + (tcp->DataOffset << 2);
			unsigned short data_length = msg_length - (tcp->DataOffset << 2);

			switch(io->State)
			{
				case TCP_STATE_LISTEN:
					if (tcp->Flags.SYN && !tcp->Flags.ACK)
					{
						// setup remote endpoint
						io->RemoteEP.IP = ip->SourceIP;
						io->RemotePort = NTOH16(tcp->SourcePort);

						if (net_ip_resolve(adapter, &io->RemoteEP))
						{
							io->Adapter = adapter;
			
							io->RcvNext = NTOH32(tcp->Sequence) + 1;
							io->SndFlags = (TCP_FLAGS) { .SYN = 1, .ACK = 1 };
							__tcp_send(io);
							io->SndNext++;
			
							io->State = TCP_STATE_SYN_RECEIVED;
						}
					}
					break;
				case TCP_STATE_SYN_RECEIVED:
					if (!tcp->Flags.SYN && tcp->Flags.ACK && NTOH32(tcp->Sequence) == io->RcvNext)
					{
						io->SndBase = io->SndAck = NTOH32(tcp->Ack);
		
						exos_event_set(&io->OutputEvent);
		
						io->SndFlags.SYN = 0;
						io->State = TCP_STATE_ESTABLISHED;
					}
					break;
				case TCP_STATE_ESTABLISHED:
					_handle_input(io, tcp, data, data_length);
					break;
				case TCP_STATE_LAST_ACK:
					if (tcp->Flags.ACK && NTOH32(tcp->Sequence) == io->RcvNext)
					{

						io->State = TCP_STATE_CLOSED;
						io->SndAck = NTOH32(tcp->Ack);
						__tcp_io_remove_io(io);
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
		// handle incoming
		if (tcp->Flags.FIN)
		{
			io->State = TCP_STATE_CLOSE_WAIT;
			io->RcvNext++;

       		io->SndFlags.ACK = 1;
			__tcp_send(io);

			// FIXME: we should avoid service to run now until application calls close();
		}
		else if (data_length != 0)
		{
			int done = exos_io_buffer_write(&io->RcvBuffer, data, data_length);
			io->RcvNext += done;

			io->SndFlags.ACK = 1;
		}

		// handle ACK
		if (tcp->Flags.ACK)
		{
			if (ack > io->SndAck && ack <= io->SndNext)
			{
				int offset = (ack - io->SndBase);
				if (offset > 0)
				{
					io->SndBase += exos_io_buffer_discard(&io->SndBuffer, offset);
				}
				io->SndAck = ack;
				// TODO: reset timeout
			}
		}

		net_tcp_service(io, 10);
	}
}

unsigned short net_tcp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset)
{
	int total_length = 0;
	unsigned long sum = 0;

	if (mbuf != NULL && offset != 0)
	{
		NET_MBUF local;
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
	sum += (sum >> 16);	// add carry to do 1's complement sum
	return (unsigned short)~sum;
}


