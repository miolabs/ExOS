#include "board.h"
#include <support/stm32f4/flash.h>
#include <support/stm32f4/dma.h>
#include <support/stm32f4/gpio.h>
#include <support/gpio_hal.h>
#include <kernel/panic.h>

static mutex_t _i2c_lock;

void __board_init()
{
	exos_mutex_create(&_i2c_lock);

	gpio_port_enable(GPIO_PORTF_A | GPIO_PORTF_B | GPIO_PORTF_C | GPIO_PORTF_D | GPIO_PORTF_E | GPIO_PORTF_F | GPIO_PORTF_G);
	dma_initialize();

#if defined BOARD_GENERIC

	#ifdef USB_FS_ENABLE_VBUS 
	// NOTE: we currently don't use VBUS input because we don't have a discharge hw (and more) 
	gpio_pin_config(PA9, GPIO_MODE_ALT_FUNC, 10);	// USB-FS VBUS
	#endif

	//gpio_pin_config(PA11, GPIO_MODE_ALT_FUNC, 10);	// USB-FS DM
	//gpio_pin_config(PA12, GPIO_MODE_ALT_FUNC, 10);	// USB-FS DP

	//gpio_pin_config(PB14, GPIO_MODE_ALT_FUNC, 12);	// USB-HS DM
	//gpio_pin_config(PB15, GPIO_MODE_ALT_FUNC, 12);	// USB-HS DP

	gpio_pin_config(PC6, GPIO_MODE_OUTPUT | GPIO_MODEF_OPEN_DRAIN, 0);
	hal_gpio_pin_set(PC6, 0);	// set /hub_reset

	//gpio_pin_config(PA1, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_REF_CLK
	//gpio_pin_config(PA7, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_CRS_DV
	//gpio_pin_config(PB11, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_TX_EN
	//gpio_pin_config(PB12, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_TXD0
	//gpio_pin_config(PB13, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_TXD1
	//gpio_pin_config(PC4, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_RXD0
	//gpio_pin_config(PC5, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_RMII_RXD1
	//gpio_pin_config(PC1, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_MDC
	//gpio_pin_config(PA2, GPIO_MODE_ALT_FUNC | GPIO_MODEF_FAST_SPEED, 11);	// ETH_MDIO

	#ifdef LED_RED_PIN
	hal_gpio_pin_config(LED_RED_PIN, GPIOF_OUTPUT);
	#endif
	#ifdef LED_GREEN_PIN
	hal_gpio_pin_config(LED_GREEN_PIN, GPIOF_OUTPUT);
	#endif

#else
#error "Board not supported"
#endif

	board_led(0);
//	board_output(BOARD_OUTF_HUB_RESET, 0);

}

void board_led(board_led_t value)
{
#ifdef LED_INVERSION_MASK
	value ^= LED_INVERSION_MASK;
#endif
#ifdef LED_RED_PIN
	hal_gpio_pin_set(LED_RED_PIN, !(value & BOARD_LEDF_RED));
#endif
#ifdef LED_GREEN_PIN
	hal_gpio_pin_set(LED_GREEN_PIN, !(value & BOARD_LEDF_GREEN));
#endif
}

unsigned board_flash_size()
{
	flash_info_t info;
	flash_get_info(&info);
	return info.TotalSize;
}

static unsigned _get_upper_sector(flash_info_t *info)
{
	unsigned upper_flash_blocks = BOOT_RESERVED_UPPER_BLOCKS;
	ASSERT(upper_flash_blocks != 0, KERNEL_ERROR_KERNEL_PANIC);

	flash_get_info(info);
	ASSERT(info->SectorCount > upper_flash_blocks, KERNEL_ERROR_NULL_POINTER);
	return (info->SectorCount - upper_flash_blocks);
}

void *board_upper_flash_addr()
{
	flash_info_t info;
	unsigned start = _get_upper_sector(&info);
	unsigned size;
	return flash_get_sector_addr(start, &size);
}

static bool _check_empty(void *base, unsigned size)
{
	unsigned *ptr = base;
	for(unsigned i = 0; i < (size >> 2); i++) if (ptr[i] != 0xffffffffu) return false;
	return true; 
}

void board_erase_upper_flash()
{
	flash_info_t info;
	unsigned start = _get_upper_sector(&info);
	for(unsigned sector = start; sector < info.SectorCount; sector++)
	{
		unsigned size;
		void *addr = flash_get_sector_addr(sector, &size);
		if (!_check_empty(addr, size))
		{
			bool done = flash_erase_sector(sector);
			ASSERT(done, KERNEL_ERROR_UNKNOWN);
		}
	}
}

