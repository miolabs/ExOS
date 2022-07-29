#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

typedef enum
{
	LED_STATUS = 0,
	LED_SDCARD,
	LED_GPS,
	LED_CAN,
	LED_AUX,
	LED_USB
} board_led_t;

typedef enum
{
	BOARD_OUTF_DCDC_EN = 1<<0,
	BOARD_OUTF_UFPWR_EN = 1<<1,
} board_output_t;


// prototypes
void board_led_set(board_led_t led, bool state);
void board_output(board_output_t mask, board_output_t value);

#endif // BOARD_H
