#ifndef NET_MACGEN_H
#define NET_MACGEN_H

#include <net/net.h>

#define MAC_OUI_LOCAL		(0x020000)
#define MAC_OUI_MULTICAST	(0x010000)
#define MAC_OUI_MASK		(~(MAC_OUI_LOCAL | MAC_OUI_MULTICAST))

void macgen_generate(hw_addr_t *addr, unsigned oui, unsigned nic);

#endif


