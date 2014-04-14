// EtherCAT Datagram Protocol Support
// by Miguel Fides

#include "ethercat.h"
#include <net/mbuf.h>
//#include "udp_io.h"
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

static const HW_ADDR _broadcast = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
 
int net_ecat_input(NET_ADAPTER *adapter, ETH_HEADER *buffer, ECAT_HEADER *ecat)
{
	// TODO
	return 0;
}

int net_ecat_build_buffer(NET_MBUF *ecat_mbuf, ECAT_HEADER *ecat, ECAT_FRAME_TYPE ecat_type, EXOS_LIST *datagram_list)
{
	int payload = 0;
	net_mbuf_init(ecat_mbuf, ecat, 0, sizeof(ECAT_HEADER));

	NET_MBUF *mbuf = ecat_mbuf;
	FOREACH(node, datagram_list)
	{
		ECAT_DATAGRAM *dgram = (ECAT_DATAGRAM *)node;
		// TODO: set bits in header according to datagram sequence

       	dgram->Header.Length = net_mbuf_length(&dgram->DataBuffer);

		net_mbuf_init(&dgram->HeaderBuffer, &dgram->Header, 0, sizeof(ECAT_DATAGRAM_HEADER));
		net_mbuf_init(&dgram->FooterBuffer, &dgram->WorkCounter, 0, 2);
		dgram->HeaderBuffer.Next = &dgram->DataBuffer;
		dgram->DataBuffer.Next = &dgram->FooterBuffer;
		dgram->FooterBuffer.Next = NULL;

		net_mbuf_append(mbuf, &dgram->HeaderBuffer);
		payload += net_mbuf_length(&dgram->HeaderBuffer);
	}
	ecat->Header = (payload & 0x7FF) | (ecat_type << 12);
	return payload;
}

int net_ecat_send(NET_ADAPTER *adapter, EXOS_LIST *datagram_list)
{
	NET_MBUF payload;
	ECAT_HEADER header;
	int payload_length = net_ecat_build_buffer(&payload, &header, ECAT_FRAME_PROTOCOL, datagram_list);
	if (payload_length != 0)
	{
		EXOS_EVENT completed_event;
		exos_event_create(&completed_event);
		NET_OUTPUT_BUFFER resp = (NET_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };
       	ECAT_HEADER *ecat = (ECAT_HEADER *)net_adapter_output(adapter, &resp, 0, 
			(HW_ADDR *)&_broadcast, ETH_TYPE_ETHERCAT);

		net_mbuf_append(&resp.Buffer, &payload);
		int done = net_adapter_send_output(adapter, &resp);
		if (done)
		{
#ifdef DEBUG
			if (exos_event_wait(&completed_event, 5000) != 0)
				kernel_panic(KERNEL_ERROR_UNKNOWN);
#else			
			exos_event_wait(&completed_event, EXOS_TIMEOUT_NEVER);
#endif
			return payload_length;
		}
	}
	return -1;
}

void net_ecat_datagram_create(ECAT_DATAGRAM *dgram, void *data, int offset, int length)
{
	dgram->Header = (ECAT_DATAGRAM_HEADER ) { .Length = length };
	net_mbuf_init(&dgram->DataBuffer, data, offset, length);
}

