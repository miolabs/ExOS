#ifndef NUC100_UART_H
#define NUC100_UART_H

#define UART_MODULE_COUNT 3

typedef volatile struct
{
	union
	{
		unsigned long RBR;
		unsigned long THR;
	};
	unsigned long IER;
	unsigned long FCR;
	unsigned long LCR;
	unsigned long MCR;
	unsigned long MSR;
	unsigned long FSR;
	union
	{
		unsigned long ISR;
		struct
		{
			unsigned char ISR_RAW;
			unsigned char ISR_STA;
			unsigned char ISR_HW_RAW;
			unsigned char ISR_HW_STA;
		};
	};
	unsigned long TOR;
	unsigned long BAUD;
	unsigned long IRCR;
	unsigned long ALT_CSR;
	unsigned long FUN_SEL;
} UART_MODULE;

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

#define UART_FCR_RFR (1 << 1)	// RX Field Reset
#define UART_FCR_TFR (1 << 2)	// TX Field Reset
#define UART_FCR_RX_TRIGGER_1 (0 << 4)
#define UART_FCR_RX_TRIGGER_4 (1 << 4)
#define UART_FCR_RX_TRIGGER_8 (2 << 4)
#define UART_FCR_RX_TRIGGER_14 (3 << 4)
#define UART_FCR_RX_TRIGGER_HS30 (4 << 4)
#define UART_FCR_RX_TRIGGER_HS46 (5 << 4)
#define UART_FCR_RX_TRIGGER_HS62 (6 << 4)
#define UART_FCR_RX_DIS (1<<8)
#define UART_FCR_RTS_TRIGGER_1 (0 << 16)
#define UART_FCR_RTS_TRIGGER_4 (1 << 16)
#define UART_FCR_RTS_TRIGGER_8 (2 << 16)
#define UART_FCR_RTS_TRIGGER_14 (3 << 16)
#define UART_FCR_RTS_TRIGGER_HS30 (4 << 16)
#define UART_FCR_RTS_TRIGGER_HS46 (5 << 16)
#define UART_FCR_RTS_TRIGGER_HS62 (6 << 16)

#define UART_IER_RDA (1 << 0)
#define UART_IER_THRE (1 << 1)
#define UART_IER_RLS (1 << 2)
#define UART_IER_MODEM (1 << 3)
#define UART_IER_RTO (1 << 4)
#define UART_IER_BUF_ERR (1 << 5)
#define UART_IER_WAKE_EN (1 << 6)
#define UART_IER_LIN_RX_BRK_IEN (1 << 8)
#define UART_IER_TIME_OUT_EN (1 << 11)
#define UART_IER_AUTO_RTS_EN (1 << 12)
#define UART_IER_AUTO_CTS_EN (1 << 13)
#define UART_IER_DMA_TX_EN (1 << 14)
#define UART_IER_DMA_RX_EN (1 << 15)

#define UART_FSR_RX_OVERFLOW (1<<0)
#define UART_FSR_RS485_ADD_DET (1<<3)
#define UART_FSR_PE (1<<4)
#define UART_FSR_FE (1<<5)
#define UART_FSR_BI (1<<6)
#define UART_FSR_RX_EMPTY (1<<14)
#define UART_FSR_RX_FULL (1<<15)
#define UART_FSR_TX_EMPTY (1<<22)
#define UART_FSR_TX_FULL (1<<23)
#define UART_FSR_TX_OVERFLOW (1<<24)
#define UART_FSR_TE_FLAG (1<<28)

#define UART_ISR_RDA (1<<0)
#define UART_ISR_THRE (1<<1)
#define UART_ISR_RLS (1<<2)
#define UART_ISR_MODEM (1<<3)
#define UART_ISR_TOUT (1<<4)
#define UART_ISR_BUF_ERR (1<<5)
#define UART_ISR_LIN_RX_BREAK (1<<7)












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

#endif // NUC100_UART_H

