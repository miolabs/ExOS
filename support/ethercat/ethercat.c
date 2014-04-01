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

void *net_ecat_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, ECAT_FRAME_TYPE type, int length)
{
	length += sizeof(ECAT_HEADER);
	ECAT_HEADER *ecat = (ECAT_HEADER *)net_adapter_output(adapter, output, length, 
		(HW_ADDR *)&_broadcast, ETH_TYPE_ETHERCAT);
	if (ecat != NULL)
	{
		ecat->Header = (length & 0x7FF) | (type << 12);
		return (void *)ecat + sizeof(ECAT_HEADER);
	}
	return NULL;
}

int net_ecat_send(NET_ADAPTER *adapter, EXOS_LIST *datagram_list)
{
	EXOS_EVENT completed_event;
	exos_event_create(&completed_event);
	NET_OUTPUT_BUFFER resp = (NET_OUTPUT_BUFFER) { .CompletedEvent = &completed_event };

	int count = 0, total = 0;
	FOREACH(node, datagram_list)
	{
		ECAT_DATAGRAM *dgram = (ECAT_DATAGRAM *)node;
		total += dgram->Header.Length + sizeof(ECAT_DATAGRAM_HEADER) + 2;
		count++;
	}

	ECAT_DATAGRAM_HEADER *header = net_ecat_output(adapter, &resp, ECAT_FRAME_PROTOCOL, total);
	if (header != NULL)
	{
		FOREACH(node, datagram_list)
		{
			ECAT_DATAGRAM *dgram = (ECAT_DATAGRAM *)node;
			*header = dgram->Header;
			
			int payload = dgram->Header.Length;
			void *data = (void *)header + sizeof(ECAT_DATAGRAM_HEADER);
			__mem_copy(data, data + payload, dgram->Data);
			unsigned short *rcnt = (unsigned short *)(data + payload);
			
			*rcnt = dgram->WorkCounter;
			header = (ECAT_DATAGRAM_HEADER *)++rcnt;
		}

		int done = net_adapter_send_output(adapter, &resp);
		if (done)
		{
#ifdef DEBUG
			if (exos_event_wait(&completed_event, 5000) != 0)
				kernel_panic(KERNEL_ERROR_UNKNOWN);
#else			
			exos_event_wait(&completed_event, EXOS_TIMEOUT_NEVER);
#endif
			return total;
		}
	}
	return -1;
}
