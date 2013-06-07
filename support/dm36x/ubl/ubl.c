
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
	int test = _memtest((void *)0x80000000, 0x10000000);
	//if (test == 0)
	//	hal_led_set(1, 1);

	/*libmem_driver_handle_t sram_handle;
	libmem_driver_handle_t ddr_handle;
	int res;

	// Register FLASH driver.
	res = libmem_register_ram_driver(&sram_handle, 
			(uint8_t *)0x00000000, 0x00004000);
	res = libmem_register_ddr_driver(&ddr_handle, 
			(uint8_t *)0x80000000, 0x10000000);

	if (res == LIBMEM_STATUS_SUCCESS)
	{
		// Run the loader 
		libmem_rpc_loader_start(buffer, buffer + sizeof(buffer) - 1);
	}

	libmem_rpc_loader_exit(res, NULL);*/

	while (1)
	{
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


