#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <usb/host.h>
#include <net/adapter.h>
#include <support/misc/eeprom.h>
#include <kernel/event.h>
#include <kernel/dispatch.h>

typedef enum
{
	BOARD_LEDF_RED = 1 << 0,
	BOARD_LEDF_GREEN = 1 << 1,
} board_led_t;

void board_led(board_led_t value);
unsigned board_flash_size();

void board_i2c_lock(bool lock);
void board_create_eeprom(eeprom_context_t *eeprom);

#endif // BOARD_H
