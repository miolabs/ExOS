#include <support/board_hal.h>
#include <support/gpio_hal.h>
#include <CMSIS/LPC11xx.h>

void hal_board_initialize()
{
#if defined BOARD_OLIMEX_P1XXX || defined BOARD_BTSMART
	LPC_IOCON->PIO2_1 = 2 | (0<<3);	// SCK1
	LPC_IOCON->PIO2_2 = 2 | (2<<3);	// MISO1
	LPC_IOCON->PIO2_3 = 2 | (0<<3);	// MOSI1
	LPC_IOCON->R_PIO1_0 = 1 | (2<<3) | (1<<7);	// RDYN
	LPC_IOCON->R_PIO1_1 = 1 | (2<<3) | (1<<7);	// REQN
#elif defined BOARD_XKUTY_CPU1
	LPC_IOCON->SCK_LOC = 2;	// SCK0 @ PIO0_6
	LPC_IOCON->PIO0_6 = 2 | (0<<3);	// SCK0
	LPC_IOCON->PIO0_8 = 1 | (2<<3);	// MISO0
	LPC_IOCON->PIO0_9 = 1 | (0<<3);	// MOSI0
	LPC_IOCON->PIO1_4 = 0 | (2<<3) | (1<<7);	// RDYN
	LPC_IOCON->PIO1_10 = 0 | (2<<3) | (1<<7);	// REQN
#else
#error Unsupported Board
#endif

#if defined BOARD_BTSMART
	hal_gpio_config(2, 1<<11, 1<<11);
	hal_gpio_pin_set(2, 11, 1);
#endif

#if defined BOARD_XKUTY_CPU1
	hal_gpio_config(0, 1<<7, 1<<7);
	hal_gpio_pin_set(0, 7, 1);
#endif
}

void hal_led_set(HAL_LED led, int state)
{

	switch(led)
	{
		case 0:
			hal_gpio_pin_set(0, 7, !state);
			break;
	}
}




