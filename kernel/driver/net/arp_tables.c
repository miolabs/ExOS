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

int net_arp_set_entry(HW_ADDR *mac, IP_ADDR *ip)
{
	ARP_ENTRY *target = NULL;

	unsigned long time = ++_time;
	unsigned long best = -1;
	for(int i = 0; i < NET_ARP_TABLE_SIZE; i++)
	{
		ARP_ENTRY *entry = &(_table[i]);
		if (entry->State == ARP_ENTRY_INVALID || net_equal_hw_addr(mac, &entry->MAC))
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
		target->MAC = *mac;
		target->IP.Value = ip->Value;
		target->State = ARP_ENTRY_VALID;
		target->Time = time;
		return 1;
	}
	return 0;
}

int net_arp_get_hw_addr(HW_ADDR *mac, IP_ADDR *ip)
{
	for(int i = 0; i < NET_ARP_TABLE_SIZE; i++)
	{
		ARP_ENTRY *entry = &(_table[i]);
		if (entry->State == ARP_ENTRY_VALID && entry->IP.Value == ip->Value)
		{
			*mac = entry->MAC;
			return 1;
		}
	}
	return 0;
}


