#include <support/misc/iap.h>
#include "cpu.h"

#ifndef IAP_BLOCK_SIZE
#define IAP_BLOCK_SIZE 512
#endif

const IAP __iap = (IAP)0x1fff1ff1;

extern int __FLASH_segment_start__;
static void *_base = (void *)&__FLASH_segment_start__;
static const FLASH_REGION _sectors[] = {{8, 0x1000}, {0, 0}};	// NOTE: for 32KB flash parts (lpc11x4)
static int _current_sector = -1;

void iap_get_flash_info(FLASH_INFO *fi)
{
	flash_fill_info(_base, (FLASH_REGION *)_sectors, fi);
	_current_sector = -1;	// none
}

int iap_write_block(FLASH_INFO *info, void *addr, const void *data)
{
	int success = 0;
	unsigned long sector_size;
	void *sector_base;
	int sector = flash_nor_find_sector(info, addr, &sector_base, &sector_size);
	if (sector >= 0)
	{
		success = 1;
		unsigned int result[2];
		IAP_RC rc;
		int cclk_khz = SystemCoreClock / 1000;
		
		if (sector != _current_sector) 
		{
			unsigned int cmd1[] = {IAP_PREPARE_SECTOR, sector, sector};
			__iap(cmd1, result);
			rc = (IAP_RC)result[0];
			if (rc != IAP_SUCCESS) success = 0;
			
			unsigned int cmd2[] = {IAP_ERASE_SECTOR, sector, sector, cclk_khz};
			__iap(cmd2, result);
			rc = (IAP_RC)result[0];
			if (rc != IAP_SUCCESS) success = 0;
		
			_current_sector = sector;
		}
		
		if (success == 1)
		{
			unsigned int cmd1[] = {IAP_PREPARE_SECTOR, 
				_current_sector, _current_sector};
			__iap(cmd1, result);
			rc = (IAP_RC)result[0];
			if (rc != IAP_SUCCESS) success = 0;
		
			unsigned int cmd2[] = {IAP_COPY_RAM_TO_FLASH, 
				(int)addr, (int)data, IAP_BLOCK_SIZE, cclk_khz};
			__iap(cmd2, result);
			rc = (IAP_RC)result[0];
			if (rc != IAP_SUCCESS) success = 0;
		}
		
	}
	return success;
}

