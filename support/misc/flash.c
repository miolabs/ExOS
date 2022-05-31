#include "flash.h"

int flash_fill_info(void *base, FLASH_REGION *first, FLASH_INFO *info)
{
	*info = (FLASH_INFO) { .Base = base, .Regions = first };
	FLASH_REGION *array = first;
	while(array->Count != 0)
	{
		info->TotalSectors += array->Count;
		info->TotalSize += (array->Count * array->Size);
		array++;
	}
	return 1;
}

int flash_nor_get_sector(FLASH_INFO *info, int sector_num, void **base, unsigned long *size)
{
	void *sector_base = info->Base;

	int sector = 0;
	FLASH_REGION *array = info->Regions;
	while(array->Count != 0)
	{
		unsigned short i;
		for (i = 0; i < array->Count; i++)
		{
			if (sector == sector_num)
			{
				*base = sector_base;
				*size = array->Size;
				return 1;
			}
			else
			{
				sector++;
				sector_base += array->Size;
			}
		}
		array++;
	}
	return 0;
}

int flash_nor_find_sector(FLASH_INFO *info, void *addr, void **base, unsigned long *size)
{
	void *sector_base = info->Base;
	if (addr >= sector_base)
	{
		int sector = 0;
		FLASH_REGION *array = info->Regions;
		while(array->Count != 0)
		{
			unsigned short i;
			for (i = 0; i < array->Count; i++)
			{
				if (addr < (sector_base + array->Size))
				{
					*base = sector_base;
					*size = array->Size;
					return sector;
				}
				else
				{
					sector++;
					sector_base += array->Size;
				}
			}
			array++;
		}
	}
    return -1;
}

int flash_nor_find_first_large_sector_base(FLASH_INFO *info, void **base, unsigned long size)
{
	void *sector_base = info->Base;
	FLASH_REGION *array = info->Regions;
	int sector = 0;
	while(array->Count != 0)
	{
		if (size <= array->Size)
		{
			*base = sector_base;
			return 1;
		}
		else
		{
			sector += array->Count;
			sector_base += (array->Size * array->Count);
		}
		array++;
	}
	return 0;
}

int flash_nor_find_last_large_sector_base(FLASH_INFO *info, void **base, unsigned long size)
{
	void *sector_base = info->Base;
	FLASH_REGION *array = info->Regions;
	int found = 0;
	int sector = 0;
	while(array->Count != 0)
	{
		if (size <= array->Size)
		{
			*base = sector_base + (array->Size * (array->Count - 1));
			found = 1;
		}
		else
		{
			sector += array->Count;
			sector_base += (array->Size * array->Count);
		}
		array++;
	}
	return found;
}


