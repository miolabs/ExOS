#ifndef BOARD_H
#define BOARD_H

#include <kernel/event.h>
#include <kernel/dispatch.h>

typedef enum
{
	BOARD_OUTF_DCDC_EN = 1<<0,
	BOARD_OUTF_UFPWR_EN = 1<<1,
} board_output_t;


void board_output(board_output_t mask, board_output_t value);
//board_input_t board_input(board_input_t mask);

void board_usb_host_start(dispatcher_context_t *context);


#endif // BOARD_H
