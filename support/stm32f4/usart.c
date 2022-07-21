#include "usart.h"
#include "cpu.h"
#include <support/uart_hal.h>
#include <kernel/iobuffer.h>
#include <kernel/panic.h>

#define UART_MODULE_COUNT 8
static uart_control_block_t *_control[UART_MODULE_COUNT];
static USART_TypeDef *const _modules[] = { USART1, USART2, 
#ifdef USART3
	USART3,
#else
	nullptr,
#endif 
#ifdef UART4
	UART4, 
#else
	nullptr,
#endif 
#ifdef UART5
	UART5, 
#else
	nullptr,
#endif 
#ifdef USART6
	USART6, 
#else
	nullptr,
#endif 
#ifdef UART7
	UART7, 
#else
	nullptr,
#endif 
#ifdef UART8
	UART8 
#else
	nullptr,
#endif 
};

static inline bool _valid_module(unsigned module, USART_TypeDef **uart, uart_control_block_t **cb)
{
	if (module < UART_MODULE_COUNT) 
	{
		*uart = _modules[module];
		*cb = _control[module];
		return (*cb != nullptr);
	}
	return false;
}

static bool _initialize(unsigned module, unsigned long baudrate)
{
	USART_TypeDef *uart;
	uart_control_block_t *cb;
	unsigned pclk;
	if (_valid_module(module, &uart, &cb))
	{
		ASSERT(uart != nullptr, KERNEL_ERROR_KERNEL_PANIC);

		switch(module)
		{
			case 0:	
				RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
				RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
				RCC->APB2RSTR ^= RCC_APB2RSTR_USART1RST;
                pclk = cpu_get_pclk2();
				break;
			case 1:	
				RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
				RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_USART2RST;
				pclk = cpu_get_pclk1();
				ASSERT(pclk < 36000000UL, KERNEL_ERROR_KERNEL_PANIC);
				break;
#ifdef USART3
			case 2:	
				RCC->APB1RSTR |= RCC_APB1RSTR_USART3RST;
				RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_USART3RST;
				pclk = cpu_get_pclk1();
				ASSERT(pclk < 36000000UL, KERNEL_ERROR_KERNEL_PANIC);
				break;
#endif
#ifdef UART4
			case 3:
				RCC->APB1RSTR |= RCC_APB1RSTR_UART4RST;
				RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_UART4RST;
				pclk = cpu_get_pclk1();
				ASSERT(pclk < 36000000UL, KERNEL_ERROR_KERNEL_PANIC);
				break;
#endif
#ifdef UART5
			case 4:
				RCC->APB1RSTR |= RCC_APB1RSTR_UART5RST;
				RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_UART5RST;
				pclk = cpu_get_pclk1();
				ASSERT(pclk < 36000000UL, KERNEL_ERROR_KERNEL_PANIC);
				break;
#endif
#ifdef USART6
			case 5:
				RCC->APB2RSTR |= RCC_APB2RSTR_USART6RST;
				RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
				RCC->APB2RSTR ^= RCC_APB2RSTR_USART6RST;
                pclk = cpu_get_pclk2();
				break;
#endif
#ifdef UART7
			case 6:
				RCC->APB1RSTR |= RCC_APB1RSTR_UART7RST;
				RCC->APB1ENR |= RCC_APB1ENR_UART7EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_UART7RST;
                pclk = cpu_get_pclk1();
				break;
#endif
#ifdef UART8
			case 7:
				RCC->APB1RSTR |= RCC_APB1RSTR_UART8RST;
				RCC->APB1ENR |= RCC_APB1ENR_UART8EN;
				RCC->APB1RSTR ^= RCC_APB1RSTR_UART8RST;
                pclk = cpu_get_pclk1();
				break;
#endif
			default:
				kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
		}

		uart->CR1 = USART_CR1_UE;	// enable
		uart->CR2 = 0;	// 1 stop bit
		// TODO: program DMA
		uart->CR1 |= USART_CR1_TE;
		uart->BRR = (pclk + (baudrate >> 1)) / baudrate;
		cb->Baudrate = pclk / uart->BRR;

		uart->CR1 |= USART_CR1_RE;	// receiver enable;
		uart->CR1 |= USART_CR1_RXNEIE | USART_CR1_TXEIE; // enable interrupts
		switch(module)
		{
			case 0:	NVIC_EnableIRQ(USART1_IRQn);	break;
			case 1:	NVIC_EnableIRQ(USART2_IRQn);	break;
#ifdef USART3
			case 2:	NVIC_EnableIRQ(USART3_IRQn);	break;
#endif
#ifdef UART4
			case 3:	NVIC_EnableIRQ(UART4_IRQn);		break;
#endif
#ifdef UART5
			case 4:	NVIC_EnableIRQ(UART5_IRQn);		break;
#endif
#ifdef USART6
			case 5:	NVIC_EnableIRQ(USART6_IRQn);	break;
#endif
#ifdef UART7
			case 6:	NVIC_EnableIRQ(UART7_IRQn);	break;
#endif
#ifdef UART8
			case 7:	NVIC_EnableIRQ(UART8_IRQn);	break;
#endif
		}
		return true;
	}
	return false;
}

bool uart_initialize(unsigned module, uart_control_block_t *cb)
{
	ASSERT(cb != nullptr, KERNEL_ERROR_NULL_POINTER);
	if (module >= UART_MODULE_COUNT)
		return false;

	if (_control[module] != nullptr)
		kernel_panic(KERNEL_ERROR_KERNEL_PANIC);	// re-initializing without de-initializing first

	_control[module] = cb;
	return _initialize(module, cb->Baudrate);
}

static void _irq(unsigned module)
{
	USART_TypeDef *uart = _modules[module];
	uart_control_block_t *cb = _control[module];
	ASSERT(cb != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	unsigned char c;
	unsigned sr;
	while(sr = uart->SR, sr & (USART_SR_TXE | USART_SR_RXNE))
	{
		if (sr & USART_SR_RXNE)
		{
			if (sr & USART_SR_ORE)
				cb->Status |= UART_RX_OVERRUN;

			c = uart->DR;
			if (0 == exos_io_buffer_write(&cb->Input, &c, 1))
			{
				cb->Status |= UART_RX_OVERRUN;
			}
			else continue;
		}
		if (sr & USART_SR_TXE)
		{
			if (0 == exos_io_buffer_read(&cb->Output, &c, 1))
			{
				uart->CR1 &= ~USART_CR1_TXEIE;
				cb->Status &= ~UART_TX_BUSY;
			}
			else
			{
				uart->DR = c;
				continue;
			}
		}
		break;
	}
}

void uart_disable(unsigned module)
{
	switch(module)
	{
		case 0:
			NVIC_DisableIRQ(USART1_IRQn);
			USART1->CR1 = 0;
			_control[0] = nullptr;
			break;
		case 1:
			NVIC_DisableIRQ(USART2_IRQn);
			USART2->CR1 = 0;
			_control[1] = nullptr;
			break;
#ifdef USART3
		case 2:
			NVIC_DisableIRQ(USART3_IRQn);
			USART3->CR1 = 0;
			_control[2] = nullptr;
			break;
#endif
#ifdef UART4
		case 3:
			NVIC_DisableIRQ(UART4_IRQn);
			UART4->CR1 = 0;
			_control[3] = nullptr;
			break;
#endif
#ifdef UART5
		case 4:
			NVIC_DisableIRQ(UART5_IRQn);
			UART5->CR1 = 0;
			_control[4] = nullptr;
			break;
#endif
#ifdef USART6
		case 5:
			NVIC_DisableIRQ(USART6_IRQn);
			USART6->CR1 = 0;
			_control[5] = nullptr;
			break;
#endif
#ifdef UART7
		case 6:
			NVIC_DisableIRQ(UART7_IRQn);
			UART7->CR1 = 0;
			_control[6] = nullptr;
			break;
#endif
#ifdef UART8
		case 7:
			NVIC_DisableIRQ(UART8_IRQn);
			UART8->CR1 = 0;
			_control[7] = nullptr;
			break;
#endif
	}
}

int uart_read(unsigned module, unsigned char *buffer, unsigned long length)
{
    USART_TypeDef *uart;
	uart_control_block_t *cb;
	if (!_valid_module(module, &uart, &cb)) return -1;

	int done = exos_io_buffer_read(&cb->Input, buffer, length);
	return done;
}

int uart_write(unsigned module, const unsigned char *buffer, unsigned long length)
{
    USART_TypeDef *uart;
	uart_control_block_t *cb;
	if (!_valid_module(module, &uart, &cb)) return -1;

	int done = exos_io_buffer_write(&cb->Output, buffer, length);

	if (!(cb->Status & UART_TX_BUSY))
	{
		cb->Status |= UART_TX_BUSY;
		uart->CR1 |= USART_CR1_TXEIE;
	}
	return done;
}


void USART1_IRQHandler()
{
	_irq(0);
}

void USART2_IRQHandler()
{
	_irq(1);
}

void USART3_IRQHandler()
{
	_irq(2);
}

void UART4_IRQHandler()
{
	_irq(3);
}

void UART5_IRQHandler()
{
	_irq(4);
}

void USART6_IRQHandler()
{
	_irq(5);
}

void UART7_IRQHandler()
{
	_irq(6);
}

void UART8_IRQHandler()
{
	_irq(7);
}
