// IP Stack. Basic ICMP Support
// by Miguel Fides

#include "icmp.h"

void net_icmp_input(ETH_ADAPTER *adapter, ETH_HEADER *eth, IP_HEADER *ip)
{
	unsigned short msg_length;
	ICMP_HEADER *icmp = net_ip_get_payload(ip, &msg_length);

	unsigned short checksum = net_ip_checksum((NET16_T *)icmp, msg_length);
	if (checksum == 0)
	{
		ETH_OUTPUT_BUFFER resp = (ETH_OUTPUT_BUFFER) { .CompletedEvent = NULL };

		IP_ENDPOINT sender_ep = (IP_ENDPOINT) { 
			.MAC = eth->Sender,
			.IP = ip->SourceIP };	// caller endpoint to send response to

		ICMP_TYPE type = icmp->Type;
		if (type == ICMP_TYPE_ECHO_REQUEST)
		{
			ICMP_HEADER *icmp_resp = (ICMP_HEADER *)net_ip_output(adapter, &resp, msg_length, &sender_ep, IP_PROTOCOL_ICMP);
			if (icmp_resp != NULL)
			{
				icmp_resp->Type = ICMP_TYPE_ECHO_REPLY;
				icmp_resp->Code = 0;
				icmp_resp->Checksum = HTON16(0);
				icmp_resp->Id = icmp->Id;
				icmp_resp->Sequence = icmp->Sequence;
	
				// copy msg data
				int data_length = msg_length - sizeof(ICMP_HEADER); 
				for(int i = 0; i < data_length; i++) icmp_resp->Data[i] = icmp->Data[i];
	
				unsigned short checksum = net_ip_checksum((NET16_T *)icmp_resp, msg_length);
				icmp_resp->Checksum = HTON16(checksum);
				net_ip_send_output(adapter, &resp, msg_length);
			}
		}
	}
}


