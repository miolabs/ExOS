// LPC2K UART Peripheral Support
// by Miguel Fides

#include "uart.h"
#include "cpu.h"
#include <kernel/panic.h>

static UART_CONTROL_BLOCK *_control[UART_MODULE_COUNT];
static LPC_UART_TypeDef *const _modules[] = { 
	LPC_UART0, LPC_UART1, LPC_UART2, LPC_UART3 };
static const PCLK_PERIPH _pclk[] = {
	PCLK_UART0, PCLK_UART1, PCLK_UART2, PCLK_UART3 };

static inline int _valid_module(unsigned module, LPC_UART_TypeDef **uart, UART_CONTROL_BLOCK **cb)
{
	if (module < UART_MODULE_COUNT) 
	{
		*uart = _modules[module];
		*cb = _control[module];
		return (*cb != (void *)0);
	}
	return 0;
}

static int _initialize(unsigned module, unsigned long baudrate)
{
	LPC_UART_TypeDef *uart;
	UART_CONTROL_BLOCK *cb;
	if (_valid_module(module, &uart, &cb))
	{
		switch(module)
		{
			case 0:	LPC_SC->PCONP |= PCONP_PCUART0;	break;
			case 1:	LPC_SC->PCONP |= PCONP_PCUART1;	break;
			case 2:	LPC_SC->PCONP |= PCONP_PCUART2;	break;
			case 3:	LPC_SC->PCONP |= PCONP_PCUART3;	break;
		}

		int pclk = cpu_pclk(_pclk[module]);
		unsigned short divisor = pclk / (16 * baudrate);
		uart->LCR = UART_LCR_WLEN_8BIT | UART_LCR_STOP_1BIT | UART_LCR_DLAB; // no parity
		uart->SCR = 0;
		uart->DLL = divisor & 0xFF;
		uart->DLM = (divisor >> 8) & 0xFF;
		uart->FDR = 1 << 4;
		cb->Baudrate = pclk / (16 * divisor);

//		unsigned int rem = pclk - (divisor * 16 * baudrate);
//		cb->Baudrate = (pclk * m) / (16 * divisor * (m + s));
	
		uart->LCR &= ~UART_LCR_DLAB; // disable DLAB
		uart->FCR = UART_FCR_FIFO_ENABLE | UART_FCR_RXFIFO_RESET | UART_FCR_TXFIFO_RESET |
			UART_FCR_RX_TRIGGER_2; // FIFO enabled, 8 char RX trigger
		uart->IER = UART_IER_RBR | UART_IER_THRE | UART_IER_RX;

		switch(module)
		{
			case 0:	VIC_EnableIRQ(UART0_IRQn);	break;
			case 1:	VIC_EnableIRQ(UART1_IRQn);	break;
			case 2:	VIC_EnableIRQ(UART2_IRQn);	break;
			case 3:	VIC_EnableIRQ(UART3_IRQn);	break;
		}

		return 1;
	}
	return 0;
}

int uart_initialize(unsigned module, UART_CONTROL_BLOCK *cb)
{
	if (module < UART_MODULE_COUNT)
	{
		if (_control[module] != (void *)0)
			kernel_panic(KERNEL_ERROR_UNKNOWN);	// re-initializing without de-initializing first

		_control[module] = cb;
	}
	return _initialize(module, cb->Baudrate);
}

void uart_disable(unsigned module)
{
	switch(module)
	{
		case 0:
			VIC_DisableIRQ(UART0_IRQn);
			break;
		case 1:
			VIC_DisableIRQ(UART1_IRQn);
			break;
		case 2:
			VIC_DisableIRQ(UART2_IRQn);
			break;
		case 3:
			VIC_DisableIRQ(UART3_IRQn);
			break;
		default:
			return;
	}

	_control[module] = (UART_CONTROL_BLOCK *)0;
}

static void _reset_receiver(LPC_UART_TypeDef *uart, UART_CONTROL_BLOCK *cb)
{
	unsigned char lsr = uart->LSR;
	uart->FCR = UART_FCR_RXFIFO_RESET;
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
		if (++count >= max) break;
	}

	if (count != 0 && cb->Handler) 
		cb->Handler(UART_EVENT_OUTPUT_READY, cb->HandlerState);
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

int uart_read(unsigned module, unsigned char *buffer, unsigned long length)
{
    LPC_UART_TypeDef *uart;
	UART_CONTROL_BLOCK *cb;
	if (!_valid_module(module, &uart, &cb)) return -1;

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

int uart_write(unsigned module, const unsigned char *buffer, unsigned long length)
{
    LPC_UART_TypeDef *uart;
	UART_CONTROL_BLOCK *cb;
	if (!_valid_module(module, &uart, &cb)) return -1;

	UART_BUFFER *output = &cb->OutputBuffer;
	int done;
	for(done = 0; done < length; done++)
	{
		int index = output->ProduceIndex + 1;
        if (index == output->Size) index = 0;
		if (index == output->ConsumeIndex)
		{
			if (cb->Handler) cb->Handler(UART_EVENT_OUTPUT_FULL, cb->HandlerState);
			break;
		}
		output->Buffer[output->ProduceIndex] = buffer[done];
		output->ProduceIndex = index;
	}

	if (done == length && cb->Handler)
	{
		int index = output->ProduceIndex + 1;
        if (index == output->Size) index = 0;
		if (index == output->ConsumeIndex)
			cb->Handler(UART_EVENT_OUTPUT_FULL, cb->HandlerState);
	}

	if (uart && (uart->LSR & UART_LSR_THRE))
		_write_data(uart, cb, 8);

	return done;
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



