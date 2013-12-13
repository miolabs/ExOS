#ifndef NET_ARP_TABLES_H
#define NET_ARP_TABLES_H

#include "arp.h"
#include <kernel/event.h>

typedef enum
{
	ARP_ENTRY_INVALID = 0,
	ARP_ENTRY_PENDING = 1,
	ARP_ENTRY_VALID = 2
} ARP_ENTRY_STATE;

typedef struct 
{
	HW_ADDR	MAC;
	short State;
	unsigned long Time;
	IP_ADDR	IP;
	EXOS_EVENT *Event;
} ARP_ENTRY;

// prototypes
int net_arp_tables_initialize();
ARP_ENTRY *net_arp_tables_set_entry(IP_ADDR *ip, HW_ADDR *mac, ARP_ENTRY_STATE state, EXOS_EVENT *event);
int net_arp_tables_get_hw_addr(IP_ADDR *ip, HW_ADDR *mac);

#endif // NET_ARP_TABLES_H
