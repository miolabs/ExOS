// IP Stack. Basic ARP Support
// by Miguel Fides

#include "arp.h"
#include "arp_tables.h"

static const HW_ADDR _arp_none = { 0, 0, 0, 0, 0, 0 };
static const HW_ADDR _arp_broadcast = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void net_arp_initialize()
{
	net_arp_tables_initialize();
}

void net_arp_input(NET_ADAPTER *adapter, ARP_HEADER *arp)
{
	ARP_PTYPE ptype = NTOH16(arp->ptype);
	if (arp->hlen == 6 && ptype == ARP_PTYPE_IPV4 && arp->plen == 4)
	{
		ARP_OPER oper = NTOH16(arp->oper);
		switch(oper)
		{
			case ARP_OPER_REQUEST:
				if ((net_equal_hw_addr(&arp->tha, (HW_ADDR *)&_arp_none) 
					|| net_equal_hw_addr(&arp->tha, &adapter->MAC))
					&& arp->tpa.Value == adapter->IP.Value)
				{
					// broadcast -> send mac to sender
					NET_OUTPUT_BUFFER resp = (NET_OUTPUT_BUFFER) { .CompletedEvent = NULL };
					ARP_HEADER *arp_resp = net_arp_output(adapter, &resp, &arp->sha);
					if (arp_resp != NULL)
					{
						arp_resp->oper = HTON16(ARP_OPER_REPLY);
						arp_resp->spa.Value = adapter->IP.Value;
						arp_resp->tha = arp->sha;
						arp_resp->tpa.Value = arp->spa.Value;
						net_arp_send_output(adapter, &resp);
					}
				}
				break;
			case ARP_OPER_REPLY:
				if (net_equal_hw_addr(&arp->tha, &adapter->MAC))
				{
					// save mac/ip in table
                    net_arp_set_hw_addr(&arp->spa, &arp->sha);
				}
				break;
		}
	}
}

ARP_HEADER *net_arp_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output, HW_ADDR *destination)
{
	ARP_HEADER *arp = (ARP_HEADER *)net_adapter_output(adapter, output, sizeof(ARP_HEADER), destination, ETH_TYPE_ARP);
	if (arp != NULL)
	{
		arp->sha = adapter->MAC;
		arp->spa = adapter->IP;
		
		arp->htype = HTON16(ARP_HTYPE_ETHERNET);
		arp->hlen = 6;
		arp->ptype = HTON16(ARP_PTYPE_IPV4);
		arp->plen = 4;
	}
	return arp;
}

int net_arp_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output)
{
	return net_adapter_send_output(adapter, output);
}

int net_arp_obtain_hw_addr(NET_ADAPTER *adapter, IP_ADDR *ip, HW_ADDR *mac)
{
	EXOS_EVENT reply_event;
	exos_event_create(&reply_event);

	int done = net_arp_tables_get_hw_addr(ip, mac);
	if (!done)
	{
		ARP_ENTRY *entry = net_arp_tables_set_entry(ip, mac, ARP_ENTRY_PENDING, &reply_event);
		if (entry != NULL)
		{
			NET_OUTPUT_BUFFER resp = (NET_OUTPUT_BUFFER) { .CompletedEvent = NULL };
			ARP_HEADER *arp = net_arp_output(adapter, &resp, (HW_ADDR *)&_arp_broadcast);
			if (arp != NULL)
			{
				arp->oper = HTON16(ARP_OPER_REQUEST);
				arp->sha = adapter->MAC;
				arp->spa.Value = adapter->IP.Value;
				arp->tha = _arp_none;
				arp->tpa.Value = ip->Value;
				net_arp_send_output(adapter, &resp);
			}

			if (-1 == exos_event_wait(&reply_event, 500))	// FIXME: allow time setup
			{
				// NOTE: timeout
				entry->Event = NULL;
			}

			done = net_arp_tables_get_hw_addr(ip, mac);
		}
	}
	return done;
}

int net_arp_set_hw_addr(IP_ADDR *ip, HW_ADDR *mac)
{
	return NULL != net_arp_tables_set_entry(ip, mac, ARP_ENTRY_VALID, NULL);
}

