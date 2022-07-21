#ifndef HAL_UART_HAL_H
#define HAL_UART_HAL_H

#include <kernel/iobuffer.h>

typedef enum
{
	UART_CTRL_CLEAR_STATUS = 0,
	UART_CTRL_CLEAR_INPUT,
	UART_CTRL_SEND_BREAK,
    UART_CTRL_CHANGE_BAUDRATE,
} uart_control_t;

typedef enum
{
	UART_RX_IDLE = 0,
	UART_RX_OVERRUN = 1<<0,
	UART_RX_BREAK = 1<<1,
	UART_RX_FLAGS = (UART_RX_OVERRUN | UART_RX_BREAK),
	UART_TX_BUSY = 1<<4,
	UART_TX_FLAGS = (UART_TX_BUSY),
} uart_status_t;

typedef enum	// NOTE: not every MCU supports all features
{
	UART_CFG_PARITY_DISABLED = (0<<0),
	UART_CFG_PARITY_EVEN = (1<<0),
	UART_CFG_PARITY_ODD = (2<<0),
	UART_CFG_PARITY_MASK = (3<<0),
	UART_CFGF_CTS_RTS = (1<<2),
	UART_CFGF_STOP_BITS_2 = (1<<3),
	UART_CFGF_INVERTED_RXD = (1<<4),
	UART_CFGF_INVERTED_TXD = (1<<5),
} uart_config_t;

#define UART_CFG_CTS_RTS UART_CFGF_CTS_RTS	// compatibility

typedef struct
{
	uart_status_t Status;
	uart_config_t Config;
	event_t *StatusEvent;
	unsigned int Baudrate;
	io_buffer_t Output;
	io_buffer_t Input;
} uart_control_block_t;

// prototypes
bool uart_initialize(unsigned module, uart_control_block_t *cb);
void uart_disable(unsigned module);
int uart_read(unsigned module, unsigned char *buffer, unsigned long length);
int uart_write(unsigned module, const unsigned char *buffer, unsigned long length);
bool uart_control(unsigned module, uart_control_t control, unsigned value);

#endif // HAL_UART_HAL_H
