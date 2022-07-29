#include "board.h"
#include <kernel/machine/hal.h>
#include <support/stm32f4/gpio.h>
#include <support/gpio_hal.h>
#include <support/stm32f4/usb_otg_host_drv.h>
#include <support/usb/driver/hid.h>

void __board_init()
{
	gpio_port_enable(GPIO_PORTF_A | GPIO_PORTF_B |GPIO_PORTF_C);

#if defined BOARD_HAMSTER

	gpio_pin_config(PA11, GPIO_MODE_ALT_FUNC, 10);	// USB-FS DM
	gpio_pin_config(PA12, GPIO_MODE_ALT_FUNC, 10);	// USB-FS DP

#else
#error "Board not supported"
#endif


}

void board_led_set(board_led_t led, bool state)
{
	// TODO
}

void board_output(board_output_t mask, board_output_t value)
{
#ifdef DCDC_EN_PIN
	if (mask & BOARD_OUTF_DCDC_EN)
		hal_gpio_pin_set(DCDC_EN_PIN, value & BOARD_OUTF_DCDC_EN);
#endif	
}

void board_usb_host_start(dispatcher_context_t *context)
{
//	usb_hub_initialize(context);
	usb_hid_initialize(context);
	usb_otg_init_as_host(context);
}


