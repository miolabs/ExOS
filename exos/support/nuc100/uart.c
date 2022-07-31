#include "uart.h"
#include "cpu.h"

static UART_CONTROL_BLOCK *_control[UART_MODULE_COUNT];
static UART_MODULE *_modules[] = { (UART_MODULE *)UART0, (UART_MODULE *)UART1, (UART_MODULE *)UART2 };

static inline int _valid_module(unsigned module, UART_MODULE **uart, UART_CONTROL_BLOCK **cb)
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
	UART_MODULE *uart;
	UART_CONTROL_BLOCK *cb;
	if (_valid_module(module, &uart, &cb))
	{
		int pclk = SystemCoreClock;

		unsigned short divisor = pclk / (16 * baudrate);
		uart->LCR = UART_LCR_WLEN_8BIT | UART_LCR_STOP_1BIT; // no parity
		uart->BAUD = divisor - 2;	// FIXME: use additional divider bits

		uart->FCR = UART_FCR_TFR | UART_FCR_RFR;	// Reset TX/RX FIFO and state machine
		uart->FCR =	UART_FCR_RX_TRIGGER_8;			// 8 char RX trigger
		uart->IER = UART_IER_RDA | UART_IER_THRE | UART_IER_RLS;

		return 1;
	}
	return 0;
}

int uart_initialize(unsigned module, UART_CONTROL_BLOCK *cb)
{
	SYSCLK->CLKSEL1.UART_S = 1;		// select common UART clock source from PLL clock
	SYSCLK->CLKDIV.UART_N = 1 - 1;	// set common clock divider for all UARTs

	switch(module)
	{
		case 0:
			SYS->GPBMFP.UART0_RX = 1;	// ALT FUNC for PB0
			SYS->GPBMFP.UART0_TX = 1;	// ALT FUNC for PB1

			SYS->IPRSTC2.UART0_RST = 1;		// assert peripheral reset
			SYSCLK->APBCLK.UART0_EN = 1;	// enable clock for UART0
			SYS->IPRSTC2.UART0_RST = 0;		// release peripheral reset
			NVIC_EnableIRQ(UART0_IRQn);
			break;
		default:
			return 0;
	}

	_control[module] = cb;
	return _initialize(module, cb->Baudrate);
}

void uart_disable(unsigned module)
{
	switch(module)
	{
		case 0:
			SYS->IPRSTC2.UART0_RST = 1;		// assert peripheral reset
			NVIC_DisableIRQ(UART0_IRQn);
			SYSCLK->APBCLK.UART0_EN = 0;	// disable clock for UART0		
			break;
		default:
			return;
	}

	_control[module] = (UART_CONTROL_BLOCK *)0;
}

static void _reset_receiver(UART_MODULE *uart, UART_CONTROL_BLOCK *cb)
{
	uart->FCR |= UART_FCR_RFR;	// Reset RX FIFO and state machine
	// FIXME: add some delay here
}

static void _read_data(UART_MODULE *uart, UART_CONTROL_BLOCK *cb)
{
	UART_BUFFER *buf = &cb->InputBuffer;
	do
	{
		int index = buf->ProduceIndex + 1;
        if (index == buf->Size) index = 0;
		if (index == buf->ConsumeIndex)
		{
			cb->Status |= UART_RX_OVERRUN;
			uart->FCR |= UART_FCR_RFR;	// Reset RX FIFO and state machine
			break;
		}
		buf->Buffer[buf->ProduceIndex] = uart->RBR;
		buf->ProduceIndex = index;
	} while(!(uart->FSR & UART_FSR_RX_FULL));

	if (cb->Handler) cb->Handler(UART_EVENT_INPUT_READY, cb->HandlerState);
}

static void _write_data(UART_MODULE *uart, UART_CONTROL_BLOCK *cb, unsigned max)
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
	UART_MODULE *uart = _modules[module];
	UART_CONTROL_BLOCK *cb = _control[module];
#ifdef DEBUG
	while(!cb);	// halt
#endif

	unsigned char c;
	int count;

	unsigned char iir;
	while (iir = uart->ISR_STA, iir != 0)
	{
		if (iir & UART_ISR_RLS)
			_reset_receiver(uart, cb);
		if (iir & UART_ISR_RDA)
			_read_data(uart, cb);
		if (iir & UART_ISR_THRE)
			_write_data(uart, cb, 16);
	}
}

int uart_read(unsigned module, unsigned char *buffer, unsigned long length)
{
    UART_MODULE *uart;
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
    UART_MODULE *uart;
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

	if (uart && (uart->FSR & UART_FSR_TX_EMPTY))
		_write_data(uart, cb, 8);

	return done;
}

void UART02_IRQHandler(void)
{
	_serve_uart(0);
	// FIXME: support uart2
}

void UART1_IRQHandler(void)
{
	_serve_uart(1);
}

void UART2_IRQHandler(void)
{
	_serve_uart(2);
}

void uart_set_rs485(unsigned module, UART_MODE mode, unsigned char dir_delay, unsigned char address)
{
//    UART_TypeDef *uart;
//	UART_CONTROL_BLOCK *cb;
//	if (_valid_module(module, &uart, &cb) && 
//		module == 0)
//	{
//		if ((mode & UART_MODE_485_DIR_ENABLE_MASK) && module == 1)
//		{
//			uart->RS485CTRL = UART_RS485CTRL_DCTRL |
//				((mode & UART_MODE_485_DIR_DTR) ? UART_RS485CTRL_SEL : 0) |
//				((mode & UART_MODE_485_DIR_INV) ? UART_RS485CTRL_OINV : 0);
//		}
//		else
//		{
//			uart->RS485CTRL = 0;
//		}
//	}
}


