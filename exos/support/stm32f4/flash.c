#include "flash.h"
#include <stm32f4xx.h>
#include <kernel/panic.h>

#if defined STM32F407xx || defined STM32F405xx
static unsigned _sector_sizes[] = { 
	0x4000, 0x4000, 0x4000, 0x4000, 0x10000,
	0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000 };
#elif defined STM32F469xx
//NOTE: 2Mbyte (2 bank) memory option (xI)
static unsigned _sector_sizes[] = { 
	0x4000, 0x4000, 0x4000, 0x4000, 0x10000,
	0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
	0x4000, 0x4000, 0x4000, 0x4000, 0x10000,
	0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000 };
#else
#error "cpu not supported"
#endif

static const unsigned _sector_count = (sizeof(_sector_sizes) / sizeof(unsigned));

void *flash_get_sector_addr(int sector, unsigned *size)
{
	if (sector < 0) 
	{
		sector += _sector_count;
		ASSERT(sector >= 0, KERNEL_ERROR_KERNEL_PANIC);
	}

	ASSERT(sector < _sector_count, KERNEL_ERROR_NOT_ENOUGH_MEMORY);
	*size = _sector_sizes[sector];
	unsigned offset = 0;
	for (unsigned i = 0; i < sector; i++) offset += _sector_sizes[i];
	return (void *)FLASH_BASE + offset;
}

bool flash_find_sector(unsigned addr, unsigned *sector)
{
	unsigned offset = addr & 0xffffff;	// NOTE: lower 24 bits, 16MByte block offset

	unsigned base = addr ^ offset;
	ASSERT(base == FLASH_BASE || base == 0, KERNEL_ERROR_KERNEL_PANIC);

	for (unsigned i = 0; i < _sector_count; i++)
	{
		unsigned sector_size = _sector_sizes[i];
		if (offset < sector_size)
		{
			*sector = i;
			return true;
		}
		offset -= sector_size;
	}
	return false;
}

static void _unlock()
{
	if (FLASH->CR & FLASH_CR_LOCK)
	{
		// unlock CR
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xCDEF89AB;
	}
	ASSERT(0 == (FLASH->CR & FLASH_CR_LOCK), KERNEL_ERROR_KERNEL_PANIC);
}

static void _clear_cache()	// NOTE: untested with caches enabled
{
	unsigned acr = FLASH->ACR;
	if (acr & (FLASH_ACR_DCEN | FLASH_ACR_ICEN))
	{
		unsigned acr_dis = acr & ~(FLASH_ACR_DCEN | FLASH_ACR_ICEN);
		FLASH->ACR = acr_dis;
		__DSB();
		FLASH->ACR = acr_dis | FLASH_ACR_DCRST | FLASH_ACR_ICRST;
		__DSB();
		FLASH->ACR = acr;
		__DSB();
	}
}

static unsigned _erase_sector(unsigned sector)
{
	ASSERT(sector < _sector_count, KERNEL_ERROR_KERNEL_PANIC);

	_unlock();
	unsigned cr = (2 << FLASH_CR_PSIZE_Pos) // 2 = X32
		| (sector << FLASH_CR_SNB_Pos)
		| FLASH_CR_SER;	// Sector ERase
	FLASH->CR = cr;
	FLASH->CR = cr | FLASH_CR_STRT;

	unsigned sta = FLASH->SR;
	while(sta & FLASH_SR_BSY) sta = FLASH->SR;
	
	_clear_cache();

	return sta;
}

void *flash_begin_write(unsigned sector, unsigned *psize, bool erase)
{
	if (erase)
	{
		unsigned sta = _erase_sector(sector);
		sta &= FLASH_SR_PGSERR | FLASH_SR_PGPERR | FLASH_SR_PGAERR;
		if (sta != 0)
			return nullptr;
	}
	else
	{
		_unlock();
		FLASH->CR = (2 << FLASH_CR_PSIZE_Pos); // 2 = X32
	}

	unsigned size;
	void *ptr = flash_get_sector_addr(sector, &size);
	ASSERT(ptr != nullptr && size != 0, KERNEL_ERROR_KERNEL_PANIC);

	FLASH->CR |= FLASH_CR_PG;
	__DSB();

	*psize = size;
	return ptr;
}

void flash_end_write()
{
	FLASH->CR = FLASH_CR_LOCK;
	__DSB();
	_clear_cache();
}

void flash_get_info(flash_info_t *info)
{
	*info = (flash_info_t) { .TotalSize = 0, .SectorCount = _sector_count };
	for(unsigned i = 0; i < _sector_count; i++)
		info->TotalSize += _sector_sizes[i];
}


