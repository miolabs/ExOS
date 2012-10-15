// LPC17xx UART Peripheral Support
// by Miguel Fides

#include "uart.h"
#include "cpu.h"

// status bits include halt bit for CTS handshake
static UART_CONTROL_BLOCK *_control[UART_MODULE_COUNT];
static LPC_UART_TypeDef *_modules[] = {
	(LPC_UART_TypeDef *)LPC_UART0, (LPC_UART_TypeDef *)LPC_UART1, LPC_UART2, LPC_UART3 };

// prototypes
//static int _fill_buffer(UART_MODULE *mod, UART_Output_Buffer *buf, int max);

static void _initialize(LPC_UART_TypeDef *uart, unsigned long baudrate)
{
	int pclk = cpu_pclk(cpu_cclk(), 1);	// PCLK is fixed to CCLK/1

	unsigned short divisor = pclk / (16 * baudrate);
	uart->LCR = UART_LCR_WLEN_8BIT | UART_LCR_STOP_1BIT | UART_LCR_DLAB; // no parity
	uart->DLL = divisor & 0xFF;
	uart->DLM = (divisor >> 8) & 0xFF;
	uart->SCR = 0;

	int rem = pclk - (divisor * 16 * baudrate);
	if (rem != 0)
	{
		int m = pclk / rem;
		int mm = 15 / m;
		int m2 = (pclk * mm) / rem;
		int s = (rem * m2) / pclk;
		//int badj = (clk * m) / (16 * divisor * (m + s)); 
		uart->FDR = m << 4 | s;
	}
	else uart->FDR = 1 << 4;

	uart->LCR &= ~UART_LCR_DLAB; // disable DLAB
	uart->FCR = UART_FCR_FIFO_ENABLE | UART_FCR_RXFIFO_RESET | UART_FCR_TXFIFO_RESET |
		UART_FCR_RX_TRIGGER_2; // FIFO enabled, 8 char RX trigger
	uart->IER = UART_IER_RBR | UART_IER_THRE | UART_IER_RX;
}

static inline LPC_UART_TypeDef *_module(unsigned module)
{
	if (module >= UART_MODULE_COUNT) return (LPC_UART_TypeDef *)0;
	return _modules[module];
}

int uart_initialize(unsigned module, unsigned long baudrate, UART_CONTROL_BLOCK *cb)
{
	switch(module)
	{
		case 0:
			PCLKSEL0bits.PCLK_UART0 = 1; // PCLK = CCLK
			NVIC_EnableIRQ(UART0_IRQn);
			break;
		case 1:
			PCLKSEL0bits.PCLK_UART1 = 1; // PCLK = CCLK
			NVIC_EnableIRQ(UART1_IRQn);
			break;
		case 2:
			PCLKSEL1bits.PCLK_UART2 = 1; // PCLK = CCLK
			LPC_SC->PCONP |= PCONP_PCUART2;
			NVIC_EnableIRQ(UART2_IRQn);
			break;
		case 3:
			PCLKSEL1bits.PCLK_UART3 = 1; // PCLK = CCLK
			LPC_SC->PCONP |= PCONP_PCUART3;
			NVIC_EnableIRQ(UART3_IRQn);
			break;
	}
	LPC_UART_TypeDef *uart = _module(module);
	if (uart)
	{
		_control[module] = cb;
		_initialize(uart, baudrate);
		return 1;
	}
    return 0;
}

int uart_set_baudrate(unsigned module, unsigned long baudrate)
{
	LPC_UART_TypeDef *uart = _module(module);
	if (uart)
	{
		_initialize(uart, baudrate);
		return 1;
	}
    return 0;
}

void uart_disable(unsigned module)
{
	switch(module)
	{
		case 0:
			NVIC_DisableIRQ(UART0_IRQn);
			break;
		case 1:
			NVIC_DisableIRQ(UART1_IRQn);
			break;
		case 2:
			NVIC_DisableIRQ(UART2_IRQn);
			break;
		case 3:
			NVIC_DisableIRQ(UART3_IRQn);
			break;
	}
    LPC_UART_TypeDef *uart = _module(module);
	if (uart)
	{
		_control[module] = (UART_CONTROL_BLOCK *)0;
	}
}

int uart_read(unsigned module, unsigned char *buffer, unsigned long length)
{
	UART_CONTROL_BLOCK *cb = _control[module];
    UART_BUFFER *input = &cb->InputBuffer;

	int done;
	for(done = 0; done < length; done++)
	{
		int index = input->ConsumeIndex;
		if (index == input->ProduceIndex) 
		{
			if (cb->Handler) cb->Handler(UART_EVENT_INPUT_EMPTY, cb->HandlerState);
			break;
		}
		buffer[done] = input->Buffer[index++];
		if (index == input->Size) index = 0;
		input->ConsumeIndex = index;
	}
	return done;
}

int uart_write(unsigned module, unsigned char *buffer, unsigned long length)
{
	UART_CONTROL_BLOCK *cb = _control[module];

	// TODO
}

static void _reset_receiver(LPC_UART_TypeDef *uart, UART_CONTROL_BLOCK *cb)
{
	// TODO
}

static void _read_data(LPC_UART_TypeDef *uart, UART_CONTROL_BLOCK *cb)
{
	UART_BUFFER *buf = &cb->InputBuffer;
	do
	{
		int index = buf->ProduceIndex + 1;
        if (index == buf->Size) index = 0;
		if (index == buf->ConsumeIndex)
		{
			cb->Status |= UART_RX_OVERRUN;
			uart->FCR = UART_FCR_RXFIFO_RESET;
			break;
		}
		buf->Buffer[buf->ProduceIndex] = uart->RBR;
		buf->ProduceIndex = index;
	} while(uart->LSR & UART_LSR_RDR);

	if (cb->Handler) cb->Handler(UART_EVENT_INPUT_READY, cb->HandlerState);
}

static void _write_data(LPC_UART_TypeDef *uart, UART_CONTROL_BLOCK *cb, unsigned max)
{
	UART_BUFFER *buf = &cb->OutputBuffer;
	unsigned count = 0;
	while (buf->ConsumeIndex != buf->ProduceIndex)
	{
		uart->THR = buf->Buffer[buf->ConsumeIndex++];
		if (buf->ConsumeIndex == buf->Size) buf->ConsumeIndex = 0;
		if (++count >= max) return;
	}

	if (cb->Handler) cb->Handler(UART_EVENT_OUTPUT_EMPTY, cb->HandlerState);
}

static void _serve_uart(int module)
{
	LPC_UART_TypeDef *uart = _modules[module];
	UART_CONTROL_BLOCK *cb = _control[module];
#ifdef DEBUG
	while(!cb);	// halt
#endif

	unsigned char c;
	int count;

	unsigned char iir;
	while (0 == ((iir = uart->IIR) & UART_IIR_IntStatus))
	{
		switch(iir & UART_IIR_IntId_MASK)
		{
			case UART_IIR_IntId_RLS:
				_reset_receiver(uart, cb);
				break;
			case UART_IIR_IntId_RDA:
			case UART_IIR_IntId_CTI:
				_read_data(uart, cb);
				break;
			case UART_IIR_IntId_THRE:
				_write_data(uart, cb, 16);
				break;
			case UART_IIR_IntId_MODEM:
				// TODO
				break;
		}
	}
}

void UART0_IRQHandler(void)
{
	_serve_uart(0);
}

void UART1_IRQHandler(void)
{
	_serve_uart(1);
}

void UART2_IRQHandler(void)
{
	_serve_uart(2);
}

void UART3_IRQHandler(void)
{
	_serve_uart(3);
}


//static void _write_char(UART_MODULE *uart, UART_Output_Buffer *buf, unsigned char ch)
//{
//	int next_index = (buf->WriteIndex + 1) % buf->Size;
//	for(int wait = 0; next_index == buf->ReadIndex; wait++)
//	{
//		if (wait > 1000000) 
//		{
//			// timeout
//			buf->Running = 0;
//			break;
//		}
//	}
//
//	buf->Data[buf->WriteIndex] = ch;
//	buf->WriteIndex = next_index;
//
//	if (!buf->Running)
//	{
//		_fill_buffer(uart, buf, 1);
//	}
//}
//
//void uart_write_char(int module, unsigned char ch)
//{
//	UART_MODULE *uart;
//	UART_Output_Buffer *buf;
//	if (_get_output_module(module, &uart, &buf))
//		_write_char(uart, buf, ch);
//}
//
//void uart_write_buf_length(int module, unsigned char *buf, int length)
//{
//	UART_MODULE *uart;
//	UART_Output_Buffer *uart_buf;
//	if (_get_output_module(module, &uart, &uart_buf))
//	{
//		for(int i = 0; i < length; i++)
//		{
//			_write_char(uart, uart_buf, *buf++);
//		}
//	}
//}
//
//static int _fill_buffer(UART_MODULE *uart, UART_Output_Buffer *buf, int max)
//{
//	while(uart->LSRbits.THRE == 0);
//	int count = 0;
//	while ((buf->ReadIndex != buf->WriteIndex) && (count < max))
//	{
//		buf->Running = 1;
//
//		uart->THR = buf->Data[buf->ReadIndex];
//		buf->ReadIndex = (buf->ReadIndex + 1) % buf->Size;
//		count++;
//	}
//
//	return count;
//}


