#include "persist.h"
#include <support/misc/iap.h>
#include <CMSIS/LPC11xx.h>

int persist_load(XCPU_PERSIST_DATA *data)
{
	FLASH_INFO fi;
    iap_get_flash_info(&fi);
	void *base;
	if (flash_nor_find_last_large_sector_base(&fi, &base, sizeof(XCPU_PERSIST_DATA)))
	{
		*data = *(XCPU_PERSIST_DATA *)base;
		if (data->Magic == XCPU_PERSIST_MAGIC)
			return 1;
	}
	return 0;
}

int persist_save(const XCPU_PERSIST_DATA *data)
{
	FLASH_INFO fi;
    iap_get_flash_info(&fi);
	void *base;
	if (flash_nor_find_last_large_sector_base(&fi, &base, sizeof(XCPU_PERSIST_DATA)))
	{
		__disable_irq();
		int done = iap_write_block(&fi, base, data);
		__enable_irq();
		return done;
	}
	return 0;
}

void persist_enter_bootloader()
{
	iap_reinvoke_isp();
}




