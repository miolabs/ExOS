#include <NUC1xx.h>
#include <kernel/thread.h>
#include <support/nuc100/uart.h>
#include <support/gpio_hal.h>
#include <stdio.h>

#define LED1_PORT 0	// port A
#define LED1_PIN 10

#define UART_BUFFER_SIZE 256
unsigned char _input_buffer[UART_BUFFER_SIZE];
unsigned char _output_buffer[UART_BUFFER_SIZE];

void main()
{
	int result;
	
	unsigned char buf[32];
	UART_CONTROL_BLOCK uart = (UART_CONTROL_BLOCK) { .Baudrate = 115200,
		.InputBuffer = (UART_BUFFER) { .Buffer = _input_buffer, .Size = UART_BUFFER_SIZE },
		.OutputBuffer = (UART_BUFFER) { .Buffer = _output_buffer, .Size = UART_BUFFER_SIZE } };
	result = uart_initialize(0, &uart);

	hal_gpio_config(LED1_PORT, 1<<LED1_PIN, 1<<LED1_PIN);
	while(1)
	{
		exos_thread_sleep(500);
		hal_gpio_pin_set(LED1_PORT, LED1_PIN, 1);
		exos_thread_sleep(500);
		hal_gpio_pin_set(LED1_PORT, LED1_PIN, 0);

//		int len = sprintf(buf, "AT\r\n");
//        uart_write(0, buf, len);
//
//		len = uart_read(0, buf, sizeof(buf));
//		if (len > 0)
//		{
//			uart_write(0, buf, len);
//		}
	}
}
