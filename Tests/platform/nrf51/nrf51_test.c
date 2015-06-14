#include <CMSIS/nrf51.h>
#include <kernel/thread.h>
//#include <support/nrf51/uart.h>
#include <stdio.h>

#if defined BOARD_PCA10000
#define LED1_PIN 21	// RED
#define LED2_PIN 22	// GREEN
#define LED3_PIN 23	// BLUE
#elif defined BOARD_MIOBOARD2
#define LED1_PIN 21
#define LED2_PIN 22
#define LED3_PIN 23
#else 
#error "Unsupported board"
#endif

#define UART_BUFFER_SIZE 256
unsigned char _input_buffer[UART_BUFFER_SIZE];
unsigned char _output_buffer[UART_BUFFER_SIZE];

__thread int j;
__thread int k = 5;

void main()
{
	j = k;
	k = 1;

	int result;
	
	unsigned char buf[32];
//	UART_CONTROL_BLOCK uart = (UART_CONTROL_BLOCK) { .Baudrate = 115200,
//		.InputBuffer = (UART_BUFFER) { .Buffer = _input_buffer, .Size = UART_BUFFER_SIZE },
//		.OutputBuffer = (UART_BUFFER) { .Buffer = _output_buffer, .Size = UART_BUFFER_SIZE } };
//	result = uart_initialize(0, &uart);

	NRF_GPIO->DIRSET = (1<<LED1_PIN) | (1<<LED2_PIN) | (1<<LED3_PIN);
	const unsigned char mask_table[] = { 0, 1, 2, 4, 7 };
	int index = 0;
	while(1)
	{
		unsigned char mask = mask_table[index];
		NRF_GPIO->OUT = (mask & 1 ? 0 : (1<<LED1_PIN)) | (mask & 2 ? 0 : (1<<LED2_PIN)) | (mask & 4 ? 0 : (1<<LED3_PIN));
		index = (++index) % sizeof(mask_table); 

		exos_thread_sleep(500);

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
