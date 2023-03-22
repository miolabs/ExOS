// IP Stack. Basic ARP Support
// by Miguel Fides

#include "arp.h"
#include "arp_tables.h"
#include "net_service.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "arp", __VA_ARGS__);

static const hw_addr_t _arp_none = { 0, 0, 0, 0, 0, 0 };
static const hw_addr_t _arp_broadcast = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void net_arp_initialize()
{
	net_arp_tables_initialize();
}

bool net_arp_input(net_adapter_t *adapter, arp_header_t *arp)
{
	net_ip_config_t *config;
	if (!net_ip_get_config(adapter, &config))
	{
		_verbose(VERBOSE_ERROR, "arp input on adapter without ip config!");
		return false;
	}

	arp_protocol_t ptype = NTOH16(arp->ptype);
	if (arp->hlen == 6 && ptype == ARP_PTYPE_IPV4 && arp->plen == 4)
	{
		arp_operation_t oper = NTOH16(arp->oper);
		switch(oper)
		{
			case ARP_OPER_REQUEST:
				if ((net_equal_hw_addr(&arp->tha, (hw_addr_t *)&_arp_none) 
					|| net_equal_hw_addr(&arp->tha, &adapter->MAC))
					&& arp->tpa.Value == config->IP.Value)
				{
					_verbose(VERBOSE_DEBUG, "who has %d.%d.%d.%d? I do!",
						arp->tpa.Bytes[0], arp->tpa.Bytes[1], arp->tpa.Bytes[2], arp->tpa.Bytes[3]);

					// broadcast -> send mac to sender
					net_buffer_t *resp = net_adapter_alloc_buffer(adapter);
					if (resp != NULL)
					{
						arp_header_t *arp_resp = net_arp_output(adapter, resp, &arp->sha);
						if (arp_resp != NULL)
						{
							arp_resp->oper = HTON16(ARP_OPER_REPLY);
							arp_resp->spa.Value = config->IP.Value;
							arp_resp->tha = arp->sha;
							arp_resp->tpa.Value = arp->spa.Value;

							bool done = net_adapter_send_output(adapter, resp);
							if (done)
								break;

							_verbose(VERBOSE_ERROR, "response output failed");
						}
						net_adapter_free_buffer(resp);
					}
					else _verbose(VERBOSE_ERROR, "could not allocate response!");
				}
				else
				{
					_verbose(VERBOSE_DEBUG, "who has %d.%d.%d.%d?",
						arp->tpa.Bytes[0], arp->tpa.Bytes[1], arp->tpa.Bytes[2], arp->tpa.Bytes[3]);
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
	// NOTE: input buffer is never queued
	return false;
}

arp_header_t *net_arp_output(net_adapter_t *adapter, net_buffer_t *output, hw_addr_t *destination)
{
	net_ip_config_t *config;
	if (!net_ip_get_config(adapter, &config))
	{
		_verbose(VERBOSE_ERROR, "arp output on adapter without ip config!");
		return NULL;
	}
	
	arp_header_t *arp = (arp_header_t *)net_output(adapter, output, sizeof(arp_header_t), destination, ETH_TYPE_ARP);
	ASSERT(arp != NULL, KERNEL_ERROR_KERNEL_PANIC);
	arp->sha = adapter->MAC;
	arp->spa = config->IP;
	
	arp->htype = HTON16(ARP_HTYPE_ETHERNET);
	arp->hlen = 6;
	arp->ptype = HTON16(ARP_PTYPE_IPV4);
	arp->plen = 4;
	return arp;
}

//int net_arp_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *output)
//{
//	return net_adapter_send_output(adapter, output);
//}

bool net_arp_obtain_hw_addr(net_adapter_t *adapter, ip_addr_t *ip, hw_addr_t *mac)
{
/*	EXOS_EVENT reply_event;
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
*/
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

bool net_arp_set_hw_addr(ip_addr_t *ip, hw_addr_t *mac)
{
	return NULL != net_arp_tables_set_entry(ip, mac, ARP_ENTRY_VALID, NULL);
}

