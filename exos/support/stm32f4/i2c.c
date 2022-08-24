#include "i2c.h"
#include "cpu.h"
#include <support/i2c_hal.h>
#include <kernel/mutex.h>
#include <kernel/panic.h>

#define I2C_MODULE_COUNT 3

typedef struct
{
	mutex_t Lock;
	i2c_context_t *Context;
	unsigned char *Ptr;
	unsigned char CmdRem;
	unsigned char WriteRem;
	unsigned char ReadRem;
} i2c_instance_t;

static i2c_instance_t _instance[I2C_MODULE_COUNT];
static I2C_TypeDef *const _modules[] = { I2C1, I2C2, I2C3 };

static inline I2C_TypeDef *_get_module(unsigned module, i2c_instance_t **pinstance)
{
	ASSERT(module < I2C_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	*pinstance = &_instance[module];
	return _modules[module];
}

void hal_i2c_initialize(unsigned module, unsigned bitrate)
{
	i2c_instance_t *instance;
	I2C_TypeDef *i2c = _get_module(module, &instance);

	unsigned pclk = cpu_get_pclk1();
	ASSERT(pclk <= 36000000U, KERNEL_ERROR_KERNEL_PANIC);
	switch(module)
	{
		case 0:	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;	break;
		case 1:	RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;	break;
		case 2:	RCC->APB1ENR |= RCC_APB1ENR_I2C3EN;	break;
	}

	i2c->CR2 = ((pclk + 500000U) / 1000000U) & I2C_CR2_FREQ;
	if (bitrate <= 100000U)
	{
		ASSERT(pclk >= 2000000U, KERNEL_ERROR_KERNEL_PANIC);
		unsigned div = pclk / (2 * bitrate);
		i2c->CCR = div & I2C_CCR_CCR;
		i2c->TRISE = ((pclk / (1000000000U / 1000U)) + 1) & I2C_TRISE_TRISE;
	}
	else
	{
		ASSERT(pclk >= 4000000U, KERNEL_ERROR_KERNEL_PANIC);
		unsigned div = pclk / (3 * bitrate);
		i2c->CCR = (div & I2C_CCR_CCR) | I2C_CCR_FS;
		i2c->TRISE = ((pclk / (1000000000U / 300U)) + 1) & I2C_TRISE_TRISE;
	}
	i2c->CR1 = I2C_CR1_PE;
	i2c->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;

	exos_mutex_create(&instance->Lock);

	switch(module)
	{
		case 0:
			NVIC_EnableIRQ(I2C1_EV_IRQn);
			NVIC_EnableIRQ(I2C1_ER_IRQn);
			break;
		case 1:
			NVIC_EnableIRQ(I2C2_EV_IRQn);
			NVIC_EnableIRQ(I2C2_ER_IRQn);
			break;
		case 2:
			NVIC_EnableIRQ(I2C3_EV_IRQn);
			NVIC_EnableIRQ(I2C3_ER_IRQn);
			break;
			
	}
}

static void _event(unsigned module)
{
	i2c_instance_t *instance;
	I2C_TypeDef *i2c = _get_module(module, &instance);

	unsigned sr1 = i2c->SR1;
	if (sr1 & I2C_SR1_SB)
	{
		ASSERT(instance->Context != nullptr, KERNEL_ERROR_NULL_POINTER);
		i2c->DR = (instance->Context->Address << 1) | 
			((instance->CmdRem != 0 || instance->WriteRem != 0) ? 0 : 1);
	}
	else if (sr1 & I2C_SR1_ADDR)
	{
		ASSERT(instance->Context != nullptr, KERNEL_ERROR_NULL_POINTER);
		unsigned sr2 = i2c->SR2;	// this clears the SR1.ADDR bit
		instance->Ptr = (instance->CmdRem != 0) ?
			instance->Context->Cmd : instance->Context->Data;
		if (instance->ReadRem > 1)
			i2c->CR1 |= I2C_CR1_ACK;
		i2c->CR2 |= I2C_CR2_ITBUFEN;
	}
	else if (sr1 & I2C_SR1_TXE)
	{
		ASSERT(instance->Context != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (instance->CmdRem != 0)
		{
			i2c->DR = *instance->Ptr++;
			if (--instance->CmdRem == 0)
				instance->Ptr = instance->Context->Data;
		}
		else if (instance->WriteRem != 0)
		{
			i2c->DR = *instance->Ptr++;
			instance->WriteRem--;
		}
		else if (instance->ReadRem != 0)
		{
			if (sr1 & I2C_SR1_BTF)
			{
				sr1 = i2c->SR1;
				i2c->CR1 |= I2C_CR1_START;	 // Send repeated start!
			}
			else
			{
				i2c->CR2 &= ~I2C_CR2_ITBUFEN;
			}
		}
		else
		{
			i2c->CR1 |= I2C_CR1_STOP;
			exos_event_set(&instance->Context->Done);
		}
	}
	else if (sr1 & I2C_SR1_RXNE)
	{
		ASSERT(instance->Context != nullptr, KERNEL_ERROR_NULL_POINTER);
		if (instance->ReadRem != 0)
		{
			*instance->Ptr++ = i2c->DR;
			instance->ReadRem--;
			switch(instance->ReadRem)
			{
				case 0:	
					exos_event_set(&instance->Context->Done);
					instance->Context = nullptr;
					break;
				case 1:	
					i2c->CR1 = (i2c->CR1 & ~I2C_CR1_ACK) | I2C_CR1_STOP;
					break;
			}
		}
	}
#ifdef DEBUG
	else if (sr1 != 0)
	{
		kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}
#endif
}

static void _error(unsigned module)
{
	i2c_instance_t *instance;
	I2C_TypeDef *i2c = _get_module(module, &instance);

	unsigned sr1 = i2c->SR1;
	if (sr1 & I2C_SR1_AF)
	{
		i2c->SR1 &= ~I2C_SR1_AF;
		i2c->CR1 |= I2C_CR1_STOP;

		if (instance->Context != nullptr)
		{
			instance->Context->Error = I2C_NACK;
			exos_event_set(&instance->Context->Done);
			instance->Context = nullptr;
		}
	}
	else
	{
		i2c->SR1 &= ~(I2C_SR1_TIMEOUT | I2C_SR1_ARLO | I2C_SR1_BERR);
		// NOTE: hw may be locked up now, a swreset may be needed

		if (instance->Context != nullptr)
		{
			instance->Context->Error = I2C_BUS_ERROR;
			exos_event_set(&instance->Context->Done);
			instance->Context = nullptr;
		}
	}
}

void I2C1_EV_IRQHandler()
{
	_event(0);
}

void I2C1_ER_IRQHandler()
{
	_error(0);
}

void I2C2_EV_IRQHandler()
{
	_event(1);
}
void I2C2_ER_IRQHandler()
{
	_error(1);
}

void I2C3_EV_IRQHandler()
{
	_event(2);
}
void I2C3_ER_IRQHandler()
{
	_error(2);
}

static void _transmit(unsigned module, i2c_context_t *context, unsigned write_len, unsigned read_len)
{
	i2c_instance_t *instance;
	I2C_TypeDef *i2c = _get_module(module, &instance);
	exos_mutex_lock(&instance->Lock);
	instance->Context = context;
	instance->Ptr = nullptr;
	instance->WriteRem = write_len;
	instance->ReadRem = read_len;
	instance->CmdRem = context->CmdLength;
	
	context->Error = I2C_OK;
	// NOTE: STOP may be set from previous operation 
	i2c->CR1 = I2C_CR1_PE | I2C_CR1_START;	// Send start!

	exos_event_wait(&context->Done, EXOS_TIMEOUT_NEVER);

	exos_mutex_unlock(&instance->Lock);
}

bool hal_i2c_write(unsigned module, i2c_context_t *context, const void *data, unsigned length)
{
	ASSERT(context != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(context->Cmd != nullptr || context->CmdLength == 0, KERNEL_ERROR_KERNEL_PANIC);
	
	context->Data = (unsigned char *)data;
	_transmit(module, context, length, 0);
	return context->Error == I2C_OK;
}

bool hal_i2c_read(unsigned module, i2c_context_t *context, void *data, unsigned length)
{
	ASSERT(context != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(context->Cmd != nullptr || context->CmdLength == 0, KERNEL_ERROR_KERNEL_PANIC);
	
	context->Data = data;
	_transmit(module, context, 0, length);
	return context->Error == I2C_OK;
}


