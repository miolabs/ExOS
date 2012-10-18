// IP Stack. Transmission Control Protocol Support
// by Miguel Fides

#include "tcp.h"
#include "udp.h"
#include "tcp_io.h"

static void _handle(ETH_ADAPTER *adapter, TCP_IO_ENTRY *io, IP_HEADER *ip, TCP_HEADER *tcp, void *data, int data_length);

//static void _get_next_msg(TCP_PCB *pcb, NET_MBUF *data);
static int _send(ETH_ADAPTER *adapter, TCP_IO_ENTRY *io,  NET_MBUF *data);

int net_tcp_input(ETH_ADAPTER *adapter, ETH_HEADER *buffer, IP_HEADER *ip)
{
	unsigned short msg_length;
	TCP_HEADER *tcp = (TCP_HEADER *)net_ip_get_payload(ip, &msg_length);
	int data_offset = tcp->DataOffset << 2;
	unsigned short data_length = msg_length - data_offset;

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
			_handle(adapter, io, ip, tcp, (void *)tcp + data_offset, data_length);
			if (io->State == TCP_STATE_CLOSED) __tcp_io_remove_io(io);
		}
	}
	return 0;
}

static void _handle(ETH_ADAPTER *adapter, TCP_IO_ENTRY *io, IP_HEADER *ip, TCP_HEADER *tcp, void *data, int data_length)
{
	unsigned long seq = NTOH32(tcp->Sequence);
	unsigned long ack = NTOH32(tcp->Ack);

	switch(io->State)
	{
		case TCP_STATE_LISTEN:
			if (tcp->Flags.SYN && !tcp->Flags.ACK)
			{
				io->RemoteEP.IP = ip->SourceIP;
				io->RemotePort = NTOH16(tcp->SourcePort);
				if (net_ip_resolve(adapter, &io->RemoteEP))
				{
					io->RcvNext = ++seq;
					io->SndFlags = (TCP_FLAGS) { .SYN = 1, .ACK = 1 };
					_send(adapter, io, NULL);
					io->SndNext++;

					io->State = TCP_STATE_SYN_RECEIVED;
				}
			}
			break;
		case TCP_STATE_SYN_RECEIVED:
			if (!tcp->Flags.SYN && tcp->Flags.ACK && seq == io->RcvNext)
			{
				io->SndBase = io->SndAck = ack;

				exos_event_set(&io->OutputEvent);

				io->SndFlags.SYN = 0;
				io->State = TCP_STATE_ESTABLISHED;
			}
			break;
		

		case TCP_STATE_ESTABLISHED:
			if (seq == io->RcvNext)
			{
				// handle incoming
				if (tcp->Flags.FIN)
				{
					io->State = TCP_STATE_CLOSE_WAIT;
					io->RcvNext++;
				}
				else if (data_length != 0)
				{
//					if (io->RcvCallback != NULL) io->RcvCallback(io, data, data_length);
					io->RcvNext += data_length;
				}

				// handle ACK
				if (tcp->Flags.ACK)
				{
					if (ack > io->SndAck && ack <= io->SndNext)
					{
						int offset = (ack - io->SndBase);
						while(io->SndBuffer != NULL && 
							(io->SndBuffer->Offset + offset) >= io->SndBuffer->Length)
						{
							// skip current buffer and call callback to notify that it is disposable
							int seek = io->SndBuffer->Length - io->SndBuffer->Offset;
							io->SndBase += seek;
							offset -= seek;
//							if (pcb->SndCallback != NULL) pcb->SndCallback(pcb, pcb->SndBuffer);
							io->SndBuffer = io->SndBuffer->Next;
						}
						
						io->SndAck = ack;
						// TODO: reset timeout
					}
				}

				// handle outgoing
				io->SndFlags.ACK = 1;

				int remaining = 0;
				if (io->SndBuffer != NULL)
				{
					remaining = net_mbuf_length(io->SndBuffer);
					remaining -= (io->SndNext - io->SndBase);
				}

				if (io->SndFlags.PSH
					|| remaining >= 512 // FIXME: use NET_TCP_MAXMSGSIZE
					|| data_length != 0) 
				{
					int msg_length = _send(adapter, io, io->SndBuffer);
					io->SndNext += msg_length;

					io->SndFlags.PSH = 0;
				}

				// FIXME: wait application action to close
				if (io->State == TCP_STATE_CLOSE_WAIT)
				{
					io->SndFlags.FIN = 1;
                     _send(adapter, io, NULL);
					io->SndNext++;
				}
			}
			break;

		case TCP_STATE_CLOSE_WAIT:
			if (tcp->Flags.ACK && seq == io->RcvNext)
			{
				io->State = TCP_STATE_CLOSED;
				io->SndAck = ack;
			}
			break;
	}
}

static int _send(ETH_ADAPTER *adapter, TCP_IO_ENTRY *io, NET_MBUF *data)
{
	EXOS_EVENT completed_event;
	exos_event_create(&completed_event);
	ETH_OUTPUT_BUFFER resp = (ETH_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };

	TCP_HEADER *tcp = net_ip_output(adapter, &resp, sizeof(TCP_HEADER), &io->RemoteEP, IP_PROTOCOL_TCP);
	if (tcp != NULL)
	{
		tcp->SourcePort = HTON16(io->LocalPort);
		tcp->DestinationPort = HTON16(io->RemotePort);
		tcp->Sequence = HTON32(io->SndNext);
		
		tcp->Ack = HTON32(io->RcvNext);
		tcp->DataOffset = sizeof(TCP_HEADER) >> 2;
		tcp->Reserved = 0;
		tcp->Flags = io->SndFlags;
		tcp->WindowSize = HTON16(io->LocalWindowSize);
		tcp->Checksum = HTON16(0);
		tcp->UrgentPtr = HTON16(0);
	
		int offset = (io->SndNext - io->SndBase);
		int payload = 0;
		NET_MBUF mbuf_seek;
		if (net_mbuf_seek(&mbuf_seek, data, offset))
		{
			payload = net_mbuf_length(&mbuf_seek);
			net_mbuf_append(&resp.Buffer, &mbuf_seek);
		}
		int tcp_length = sizeof(TCP_HEADER) + payload;
	
		int tcp_offset = (unsigned char *)tcp - (unsigned char *)resp.Buffer.Buffer; 
		unsigned short checksum = net_tcp_checksum(&adapter->IP, &io->RemoteEP.IP, &resp.Buffer, tcp_offset);
		tcp->Checksum = HTON16(checksum);
		int done = net_ip_send_output(adapter, &resp, tcp_length);
		// TODO: handle failure
	
		return payload;
	}
	return -1;
}

unsigned short net_tcp_checksum(IP_ADDR *source_ip, IP_ADDR *dest_ip, NET_MBUF *mbuf, int offset)
{
	int total_length = 0;
	unsigned long sum = 0;
	while(mbuf != NULL)
	{
		int length = mbuf->Length - (mbuf->Offset + offset);
		int word_count = length >> 1;
		NET16_T *data = (NET16_T *)(mbuf->Buffer + mbuf->Offset + offset);
		if (length & 1)
		{
			sum += ((unsigned char *)data)[length - 1] << 8;
		}
		for(int i = 0; i < word_count; i++)
		{
			sum += NTOH16(data[i]);
		}
		total_length += length;
		mbuf = mbuf->Next;
		offset = 0;
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


