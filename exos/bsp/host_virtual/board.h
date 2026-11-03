#ifndef EXOS_BSP_HOST_VIRTUAL_BOARD_H
#define EXOS_BSP_HOST_VIRTUAL_BOARD_H

// Virtual board API for the host port.
//
// This is the BSP (board support package) an ExOS application links against
// when it targets the host "virtual board". It mirrors the shape of a real
// board header (LEDs, discrete outputs) so application code written against it
// can be moved to actual hardware by swapping the BSP, not the app.
//
// The default implementations (board_virtual.c) drive nothing -- LEDs and
// outputs are no-ops -- but they are declared __weak, so an application can
// override any of them with its own definition.

#include <stdbool.h>

typedef enum
{
	LED_STATUS = 0,
	LED_SDCARD,
	LED_GPS,
	LED_CAN,
	LED_AUX,
	LED_USB,
	LED_COUNT
} board_led_t;

typedef enum
{
	BOARD_OUTF_DCDC_EN = 1 << 0,
	BOARD_OUTF_UFPWR_EN = 1 << 1,
} board_output_t;

// prototypes
void board_led_set(board_led_t led, bool state);
void board_output(board_output_t mask, board_output_t value);

#endif // EXOS_BSP_HOST_VIRTUAL_BOARD_H
