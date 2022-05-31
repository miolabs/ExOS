#ifndef HAL_UART_HAL_H
#define HAL_UART_HAL_H

typedef enum
{
	UART_EVENT_ERROR = 0,
	UART_EVENT_INPUT_READY,
	UART_EVENT_INPUT_EMPTY,
	UART_EVENT_OUTPUT_FULL,
	UART_EVENT_OUTPUT_READY,
	UART_EVENT_OUTPUT_EMPTY,
	UART_EVENT_MODEM,
} UART_EVENT;

typedef enum
{
	UART_RX_IDLE = 0,
	UART_RX_OVERRUN = 1<<0,
	UART_RX_FLAGS = (UART_RX_OVERRUN),
	UART_TX_BUSY = 1<<4,
	UART_TX_FLAGS = (UART_TX_BUSY),
	UART_FLOW_NONE = 0,
	UART_FLOW_HW = 1<<8,	// CTS+RTS
	UART_FLOW_FLAGS = (UART_FLOW_HW),
} UART_STATUS;

typedef void (*UART_HANDLER)(UART_EVENT event, void *state);

typedef struct
{
	unsigned char *Buffer;
	unsigned short Size;
	volatile unsigned short ProduceIndex;
	volatile unsigned short ConsumeIndex;
} UART_BUFFER;

typedef struct
{
	UART_STATUS Status;
	unsigned int Baudrate;
	UART_HANDLER Handler;
	void *HandlerState;
	UART_BUFFER InputBuffer;
	UART_BUFFER OutputBuffer;
} UART_CONTROL_BLOCK;

// prototypes
int uart_initialize(unsigned module, UART_CONTROL_BLOCK *cb);
void uart_disable(unsigned module);
int uart_read(unsigned module, unsigned char *buffer, unsigned long length);
int uart_write(unsigned module, const unsigned char *buffer, unsigned long length);

#endif // HAL_UART_HAL_H



