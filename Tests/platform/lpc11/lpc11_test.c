#include <CMSIS/LPC11xx.h>
#include <kernel/thread.h>
#include <support/lpc11/uart.h>
#include <stdio.h>

#if defined BOARD_MIORELAY1
#define LED1_MASK (1<<6)
#define LED2_MASK (1<<7)
#define LED_PORT LPC_GPIO2
#elif defined BOARD_BMS01
#define LED1_MASK (1<<9)
#define LED2_MASK 0
#define LED_PORT LPC_GPIO2
#else 
#error "Unsupported board"
#endif

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

	LED_PORT->DIR |= LED1_MASK | LED2_MASK;
	while(1)
	{
		exos_thread_sleep(100);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = LED1_MASK;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = 0;
		exos_thread_sleep(100);
		LED_PORT->MASKED_ACCESS[LED1_MASK] = 0;
		LED_PORT->MASKED_ACCESS[LED2_MASK] = LED2_MASK;

		int len = sprintf(buf, "AT\r\n");
        uart_write(0, buf, len);

		len = uart_read(0, buf, sizeof(buf));
		if (len > 0)
		{
			uart_write(0, buf, len);
		}
	}
}
