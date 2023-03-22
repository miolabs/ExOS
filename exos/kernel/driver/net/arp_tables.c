// IP Stack. Simple ARP Tables Management
// by Miguel Fides

#include "arp_tables.h"

#define NET_ARP_TABLE_SIZE 16

static ARP_ENTRY _table[NET_ARP_TABLE_SIZE];
static unsigned long _time = 0;

int net_arp_tables_initialize()
{
	for(int i = 0; i < NET_ARP_TABLE_SIZE; i++)
	{
		ARP_ENTRY *curr = &(_table[i]);
		curr->State = ARP_ENTRY_INVALID;
		curr->IP.Value = 0;
	}
}


ARP_ENTRY *net_arp_tables_set_entry(ip_addr_t *ip, hw_addr_t *mac, ARP_ENTRY_STATE state, event_t *event)
{
	ARP_ENTRY *target = NULL;
	int matches_pending = 0;

	unsigned long time = ++_time;
	unsigned long best = -1;
	for(int i = 0; i < NET_ARP_TABLE_SIZE; i++)
	{
		ARP_ENTRY *entry = &(_table[i]);
		if (entry->State == ARP_ENTRY_INVALID || 
			ip->Value == entry->IP.Value)
		{
			target = entry;
			break;
		}
		else
		{
			unsigned long age = time - entry->Time;
			if (age < best)
			{
				target = entry;
				best = age;
			}
		}
	}

	if (target != NULL)
	{
		event_t *pending_event = target->State == ARP_ENTRY_PENDING ? target->Event : NULL;
		
		target->MAC = *mac;
		target->IP.Value = ip->Value;
		target->State = state;
		target->Time = time;
		target->Event = event;

		if (state == ARP_ENTRY_VALID && pending_event != NULL)
			exos_event_set(pending_event);
	}
	return target;
}

bool net_arp_tables_get_hw_addr(ip_addr_t *ip, hw_addr_t *mac)
{
	for(int i = 0; i < NET_ARP_TABLE_SIZE; i++)
	{
		ARP_ENTRY *entry = &(_table[i]);
		if (entry->State == ARP_ENTRY_VALID && entry->IP.Value == ip->Value)
		{
			*mac = entry->MAC;
			return true;
		}
	}
	return false;
}

