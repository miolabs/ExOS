#include "phone_manager.h"

#include <kernel/machine/hal.h>


void phone_manager_confirm (XCPU_PHONE_REG* phones)
{
	// Check if the phone already exist
	int i, dup = 0;
	for (i = 0; i < XCPU_VIEW_PHONES; i++)
		if (phones[i].flags)
			if (__str_comp(phones[i].name, phones[XCPU_NEW_PHONE].name) == 0)
				dup = 1;
	// Find 1s free slot and copy the candidate there
	if(!dup)
	{
		int found = -1;
		for (i = 0; i < XCPU_VIEW_PHONES; i++)
			if(phones[i].flags == 0)
			{
				found = 1;
				break;
			}
		if (found >= 0)
			phones[i] = phones[XCPU_NEW_PHONE];
	}
}

int phone_name_cheksum (XCPU_PHONE_REG* phone)
{
	int i, c = 0;
	for(i=0; i<XCPU_PHONE_NAME_LEN; i++)
		c += phone->name[i] ^ i;
	return c & 0xff;
}

int phone_manager_count (XCPU_PHONE_REG* phones) 
{
	int i, c = 0;
	for(i=0; i<XCPU_VIEW_PHONES; i++)	// Only XCPU_VIEW_PHONES options on screen
		if (phones[i].flags & 1)
			c++;
	return c;
}


void phone_manager_remove (XCPU_PHONE_REG* phones, int entry, unsigned int checksum)
{
	int i;
	entry = entry % XCPU_VIEW_PHONES;
	if (phone_name_cheksum(&phones[entry]) == checksum)
	{
		if (entry < (XCPU_VIEW_PHONES - 1))
			for (i=entry; i<XCPU_VIEW_PHONES; i++)
				phones[i] = phones[i + 1]; 
		phones[XCPU_VIEW_PHONES - 1].flags = 0;
		phones[XCPU_VIEW_PHONES - 1].name[0] = 0;
	}
}