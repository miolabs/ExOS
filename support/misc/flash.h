#ifndef FLASH_H
#define FLASH_H

typedef struct
{
	unsigned long Count;
	unsigned long Size;
} FLASH_REGION;

typedef struct
{
	void *Base;
	unsigned long TotalSize;
	unsigned short TotalSectors; 
	unsigned short Manufacturer;
	FLASH_REGION *Regions;
} FLASH_INFO;

#define FLASH_MANUFACTURER_UNKNOWN 0

int flash_fill_info(void *base, FLASH_REGION *first, FLASH_INFO *info);

// prototypes
int flash_nor_get_sector(FLASH_INFO *info, int sector, void **base, unsigned long *size);
int flash_nor_find_sector(FLASH_INFO *info, void *addr, void **base, unsigned long *size);
int flash_nor_find_first_large_sector_base(FLASH_INFO *info, void **base, unsigned long size);
int flash_nor_find_last_large_sector_base(FLASH_INFO *info, void **base, unsigned long size);

#endif // FLASH_H


