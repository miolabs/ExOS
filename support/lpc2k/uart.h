#ifndef LPC2K_UART_H
#define LPC2K_UART_H

#include <support/uart_hal.h>

#define UART_MODULE_COUNT 4

// NOTE: back-ported from lpc17-CMSIS
typedef struct
{
	union {
	volatile unsigned char RBR;
	volatile unsigned char THR;
	volatile unsigned char DLL;
		unsigned long RESERVED0;
	};
	union {
	volatile unsigned char DLM;
	volatile unsigned long IER;
  };
	union {
	volatile  unsigned long IIR;
	volatile  unsigned char  FCR;
	};
	volatile unsigned char  LCR;
		unsigned char  RESERVED1[7];
	volatile  unsigned char  LSR;
		unsigned char  RESERVED2[7];
	volatile unsigned char  SCR;
		unsigned char  RESERVED3[3];
	volatile unsigned long ACR;
	volatile unsigned char  ICR;
		unsigned char  RESERVED4[3];
	volatile unsigned char  FDR;
		unsigned char  RESERVED5[7];
	volatile unsigned char  TER;
		unsigned char  RESERVED6[39];
	volatile  unsigned char  FIFOLVL;
} LPC_UART_TypeDef;

#define LPC_UART0_BASE	0xE000C000
#define LPC_UART1_BASE	0xE0010000
#define LPC_UART2_BASE	0xE0078000
#define LPC_UART3_BASE	0xE007C000
#define LPC_UART0 ((LPC_UART_TypeDef *)LPC_UART0_BASE)
#define LPC_UART1 ((LPC_UART_TypeDef *)LPC_UART1_BASE)
#define LPC_UART2 ((LPC_UART_TypeDef *)LPC_UART2_BASE)
#define LPC_UART3 ((LPC_UART_TypeDef *)LPC_UART3_BASE)

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


#endif // LPC2K_UART_H

