#include <support/board_hal.h>
#include <support/gpio_hal.h>
#include <CMSIS/LPC11xx.h>

void hal_board_initialize()
{
#if defined BOARD_OLIMEX_P1XXX
	LPC_IOCON->PIO2_1 = 2 | (2<<3);	// SCK1
	LPC_IOCON->PIO2_2 = 2 | (2<<3);	// MISO1
	LPC_IOCON->PIO2_3 = 2 | (2<<3);	// MOSI1
	LPC_IOCON->R_PIO1_0 = 1 | (2<<3) | (1<<7);	// RDYN
	LPC_IOCON->R_PIO1_1 = 1 | (2<<3) | (1<<7);	// REQN
#else
#error Unsupported Board
#endif
}



