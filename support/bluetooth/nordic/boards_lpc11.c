#include <support/board_hal.h>
#include <support/gpio_hal.h>
#include <CMSIS/LPC11xx.h>

void hal_board_initialize()
{
#if defined BOARD_OLIMEX_P1XXX
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
}



