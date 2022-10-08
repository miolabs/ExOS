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
	hw_addr_t MAC;
	short State;
	unsigned long Time;
	ip_addr_t IP;
	event_t *Event;
} ARP_ENTRY;

// prototypes
int net_arp_tables_initialize();
ARP_ENTRY *net_arp_tables_set_entry(ip_addr_t *ip, hw_addr_t *mac, ARP_ENTRY_STATE state, event_t *event);
int net_arp_tables_get_hw_addr(ip_addr_t *ip, hw_addr_t *mac);

#endif // NET_ARP_TABLES_H
