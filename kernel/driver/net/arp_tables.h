#ifndef NET_ARP_TABLES_H
#define NET_ARP_TABLES_H

#include "arp.h"

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
} ARP_ENTRY;

// prototypes
int net_arp_tables_initialize();
int net_arp_set_entry(HW_ADDR *mac, IP_ADDR *ip);
int net_arp_get_hw_addr(HW_ADDR *mac, IP_ADDR *ip);

#endif // NET_ARP_TABLES_H
