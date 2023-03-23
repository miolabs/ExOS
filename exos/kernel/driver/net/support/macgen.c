#include "macgen.h"
#include <support/random_hal.h>

void macgen_generate(hw_addr_t *addr, unsigned oui, unsigned nic)
{
	if (nic == 0)
	{
		hal_random_context_t rnd;
		hal_random_context_create(&rnd, -1);
		
		hal_random_read(&rnd, (unsigned char *)&nic, sizeof(nic));
	} 
	addr->Bytes[0] = oui >> 16;
	addr->Bytes[1] = oui >> 8;
	addr->Bytes[2] = oui;
	addr->Bytes[3] = nic >> 16;
	addr->Bytes[4] = nic >> 8;
	addr->Bytes[5] = nic;
}


