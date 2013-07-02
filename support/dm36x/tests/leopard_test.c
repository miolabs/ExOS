
#include <kernel/thread.h>
#include <support/board_hal.h>
#include <libmem_loader.h>
//#include "driver.h"

static int _memtest(void *base, int size);

static unsigned char buffer[256];

int main()
{
	hal_board_initialize();

	hal_led_set(0, 1);

	// DDR RAM, 256MB
	// Small test, 1/4 MB at the start, another at the end							  
	int test = _memtest((void *)0x80000000, 0x40000);
	test    |= _memtest((void *)0x8f000000, 0x40000);
	if (test == 0)
		hal_led_set(1, 1);

	while (1)
	{
		exos_thread_sleep (500);
		hal_led_set(1, 0);
		exos_thread_sleep (500);
		hal_led_set(1, 1);
	}
	
	return 0;
}

static int _memtest(void *base, int size)
{
	unsigned int i;
	// 32 bits access
	for (i = 0; i < (size >> 2); i++)
	{
		((unsigned int *)base)[i] = i;
	}
	i = 0;
	for (i = 0; i < (size >> 2); i++)
	{
		if (((unsigned int *)base)[i] != i)
		{
			return -1;
		}
	}

	// 16 bits access
	for (i = 0; i < 0x8000; i++)
	{
		((unsigned short *)base)[i] = i % 137;
	}
	for (i = 0; i < 0x8000; i++)
	{
		if (((unsigned short *)base)[i] != i % 137)
		{
			return -2;
		}
	}
	// 8 bits access
	for ( i = 0; i < 0x100; i++)
	{
		((unsigned char *)base)[i] = i % 13;
	}
	for (i = 0; i < 0x100; i++)
	{
		if (((unsigned char *)base)[i] != i % 13)
		{
			return -3;
		}
	}
	return 0;
}


