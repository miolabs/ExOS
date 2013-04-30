#ifndef XCPU1_BOARD_H
#define XCPU1_BOARD_H

typedef enum
{
	OUTPUT_NONE = 0,
	OUTPUT_HEADL = (1<<0),
	OUTPUT_TAILL = (1<<1),
	OUTPUT_BRAKEL = (1<<2),
	OUTPUT_HORN = (1<<3),
	OUTPUT_EBRAKE = (1<<4),
} XCPU_OUTPUT_MASK;

typedef enum
{
	INPUT_NONE = 0,
	INPUT_BUTTON_START = (1<<0),
} XCPU_INPUT_MASK;

void xcpu_board_output(XCPU_OUTPUT_MASK mask);
XCPU_INPUT_MASK xcpu_board_input(XCPU_INPUT_MASK mask);
void xcpu_board_led(int led);

#endif // XCPU1_BOARD_H

