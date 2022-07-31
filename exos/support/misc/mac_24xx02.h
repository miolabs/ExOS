#ifndef MAC_24xx02_H
#define MAC_24xx02_H

#ifndef EXOS_NO_NET
#include <net/adapter.h>
#else
typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[6];
} HW_ADDR;
#endif

int mac_24xx02_get(HW_ADDR *mac);





#endif // MAC_24xx02_H


