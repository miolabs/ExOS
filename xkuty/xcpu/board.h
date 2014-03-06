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
	OUTPUT_HIGHBL = (1<<5),
	OUTPUT_LEFTL = (1<<6),
	OUTPUT_RIGHTL = (1<<7),
	OUTPUT_POWEREN = (1<<8),
	OUTPUT_MOTOREN = (1<<9),
	OUTPUT_AUX = (1<<10),
} XCPU_OUTPUT_MASK;

typedef enum
{
	INPUT_NONE = 0,
	INPUT_STOP_BUT = (1<<0),
	INPUT_START_BUT = (1<<1),
	INPUT_STAND = (1<<2),
	INPUT_ERROR = (1<<3),
	INPUT_TURN_LEFT = (1<<4),
	INPUT_TURN_RIGHT = (1<<5),
} XCPU_INPUT_MASK;

void xcpu_board_output(XCPU_OUTPUT_MASK mask);
XCPU_INPUT_MASK xcpu_board_input(XCPU_INPUT_MASK mask);
void xcpu_board_led(int led);

#endif // XCPU1_BOARD_H

