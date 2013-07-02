#ifndef HAL_BOARD_H
#define HAL_BOARD_H

typedef enum
{
	HAL_RESOURCE_GPIO = 0,
	HAL_RESOURCE_UART,
	HAL_RESOURCE_I2C,
	HAL_RESOURCE_SSP,
	HAL_RESOURCE_MCI,
	HAL_RESOURCE_CAN,
	HAL_RESOURCE_EMAC,
	HAL_RESOURCE_USBDEV,
	HAL_RESOURCE_USBHOST,
	HAL_RESOURCE_ADC,
	HAL_RESOURCE_DAC,
	HAL_RESOURCE_PWM,
	HAL_RESOURCE_CAP,
	HAL_RESOURCE_MAT,
	HAL_RESOURCE_TVOUT,
} HAL_RESOURCE;

typedef enum
{
	LED_STATUS = -128,
	LED_SDCARD,
	LED_GPS,
	LED_CAN,
	LED_AUX,
} HAL_LED;

// prototypes
void hal_board_initialize() __attribute__((__weak__));
int hal_board_init_pinmux(HAL_RESOURCE res, int unit);
void hal_led_set(HAL_LED led, int state);

#endif // HAL_BOARD_H
