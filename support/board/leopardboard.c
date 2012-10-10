#include <support/board_hal.h>
#include <support/dm36x/gpio.h>

// Leopardboard 365 leds are at GIO58, GIO57, active high
#define LED1 58	// GIO58
#define LED2 57 // GIO57

void hal_board_initialize()
{
	gpio_initialize();
   	gpio_setup(LED1, GPIO_DIR_OUTPUT, 0);
	gpio_setup(LED2, GPIO_DIR_OUTPUT, 0);
}

int hal_board_init_pinmux(HAL_RESOURCE res, int unit)
{
	return 0;
}

void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
			gpio_set(LED1, state);
			break;
		case 1:
			gpio_set(LED2, state);
			break;
	}
}
