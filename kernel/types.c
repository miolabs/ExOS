#include "types.h"
#include "panic.h"

GUID GUID_NULL = { /* all zeros */ };

bool __guid_eq(GUID *guid1, GUID *guid2)
{
	if (guid1 == NULL || guid2 == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	for(int i = 0; i < 16; i++) 
		if (guid1->Bytes[i] != guid2->Bytes[i])
			return false;
	
	return true;
}
