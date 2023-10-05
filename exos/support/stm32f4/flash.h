#ifndef STM32F4_FLASH_H
#define STM32F4_FLASH_H

#include <stdbool.h>

typedef struct
{
	unsigned TotalSize;
	unsigned SectorCount;
} flash_info_t;

void *flash_get_sector_addr(int sector, unsigned *size);
bool flash_find_sector(unsigned addr, unsigned *sector);
void *flash_begin_write(unsigned sector, unsigned *psize, bool erase);
void flash_end_write();
void flash_get_info(flash_info_t *info);
bool flash_erase_sector(unsigned sector);

#endif // STM32F4_FLASH_H
