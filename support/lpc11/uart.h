#ifndef LPC_UART_H
#define LPC_UART_H

#define UART_MODULE_COUNT 1

#define UART_LCR_WLEN_5BIT (0 << 0)
#define UART_LCR_WLEN_6BIT (1 << 0)
#define UART_LCR_WLEN_7BIT (2 << 0)
#define UART_LCR_WLEN_8BIT (3 << 0)
#define UART_LCR_STOP_1BIT (0 << 2)
#define UART_LCR_STOP_2BIT (1 << 2)
#define UART_LCR_PARITY_ENABLE (1 << 3)
#define UART_LCR_PARITY_ODD (0 << 4)
#define UART_LCR_PARITY_EVEN (1 << 4)
#define UART_LCR_PARITY_STICK1 (2 << 4)
#define UART_LCR_PARITY_STICK0 (3 << 4)
#define UART_LCR_BREAK (1 << 6)
#define UART_LCR_DLAB (1 << 7)
 
#define UART_FCR_FIFO_ENABLE (1 << 0)
#define UART_FCR_RXFIFO_RESET (1 << 1)
#define UART_FCR_TXFIFO_RESET (1 << 2)
#define UART_FCR_RX_TRIGGER_0 (0 << 6)
#define UART_FCR_RX_TRIGGER_1 (1 << 6)
#define UART_FCR_RX_TRIGGER_2 (2 << 6)
#define UART_FCR_RX_TRIGGER_3 (3 << 6)

#define UART_IER_RBR (1 << 0)
#define UART_IER_THRE (1 << 1)
#define UART_IER_RX (1 << 2)
#define UART_IER_MODEM (1 << 3)
#define UART_IER_CTS (1 << 7)

#define UART_IIR_IntStatus (1 << 0)
#define UART_IIR_IntId_MASK (7 << 1)
#define UART_IIR_IntId_RLS (3 << 1)
#define UART_IIR_IntId_RDA (2 << 1)
#define UART_IIR_IntId_CTI (6 << 1)
#define UART_IIR_IntId_THRE (1 << 1)
#define UART_IIR_IntId_MODEM (0 << 1)

#define UART_LSR_RDR (1 << 0)
#define UART_LSR_OE (1 << 1)
#define UART_LSR_PE (1 << 2)
#define UART_LSR_FE (1 << 3)
#define UART_LSR_BI (1 << 4)
#define UART_LSR_THRE (1 << 5)
#define UART_LSR_TEMT (1 << 6)
#define UART_LSR_RXFE (1 << 7)

#define UART_RS485CTRL_NMMEN (1<<0)
#define UART_RS485CTRL_RXDIS (1<<1)
#define UART_RS485CTRL_AADEN (1<<2)
#define UART_RS485CTRL_SEL (1<<3)
#define UART_RS485CTRL_DCTRL (1<<4)
#define UART_RS485CTRL_OINV (1<<5)


typedef enum
{
	UART_MODE_STANDARD = 0,
	UART_MODE_485_DIR_RTS = 1,
	UART_MODE_485_DIR_DTR = 2,
	UART_MODE_485_DIR_ENABLE_MASK = (UART_MODE_485_DIR_RTS | UART_MODE_485_DIR_RTS),
	UART_MODE_485_DIR_INV = 1<<2,
	UART_MODE_485_ADDR_DETECT = 1<<3,
} UART_MODE;

typedef enum
{
	UART_EVENT_ERROR = 0,
	UART_EVENT_INPUT_READY,
	UART_EVENT_INPUT_EMPTY,
	UART_EVENT_OUTPUT_FULL,
	UART_EVENT_OUTPUT_READY,
	UART_EVENT_MODEM,
} UART_EVENT;

typedef enum
{
	UART_RX_IDLE = 0,
	UART_RX_OVERRUN = 1<<0,
	UART_RX_FLAGS = (UART_RX_OVERRUN),
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
	unsigned long Baudrate;
	UART_STATUS Status;
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
void uart_set_rs485(unsigned module, UART_MODE mode, unsigned char dir_delay, unsigned char address);

#endif // LPC_UART_H

