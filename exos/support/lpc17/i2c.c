// LPC17xx I2C Peripheral Support
// by Miguel Fides

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

static inline i2c_module_t *_get_module(unsigned module, i2c_instance_t **pinstance)
{
	*pinstance = &_instance[module];
	switch(module)
	{
		case 0:	return (i2c_module_t *)LPC_I2C0_BASE;
		case 1:	return (i2c_module_t *)LPC_I2C1_BASE;
		case 2:	return (i2c_module_t *)LPC_I2C2_BASE;
	}
	kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
}

void hal_i2c_initialize(unsigned module, unsigned bitrate)
{
	unsigned pclk;
	i2c_instance_t *instance;
	i2c_module_t *i2c = _get_module(module, &instance);
	unsigned irq;
	switch(module)
	{
		case 0:
			LPC_SC->PCONP |= PCONP_PCI2C0;
			pclk = cpu_pclk(PCLK_I2C0);
			irq = I2C0_IRQn;
			break;
		case 1:
			LPC_SC->PCONP |= PCONP_PCI2C1;
			pclk = cpu_pclk(PCLK_I2C1);
			irq = I2C1_IRQn;
			break;
		case 2:
			LPC_SC->PCONP |= PCONP_PCI2C2;
			pclk = cpu_pclk(PCLK_I2C2);
			irq = I2C2_IRQn;
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}

	i2c->CONCLR = 0xFF;
	unsigned third_divider = pclk / (bitrate * 3);
	i2c->SCLH = third_divider;
	i2c->SCLL = third_divider * 2;
	i2c->CONSET = I2C_CON_I2EN;	// enable module

	exos_mutex_create(&instance->Lock);
	instance->Context = nullptr;
	NVIC_EnableIRQ(irq);
}

static void _irq(unsigned module)
{
	i2c_instance_t *instance;
	i2c_module_t *i2c = _get_module(module, &instance);
	i2c_context_t *context = instance->Context;
	ASSERT(context != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	
	switch(i2c->STAT)
	{
		case 0x08:	// start transmitted
		case 0x10:	// restart transmitted
			if (instance->CmdRem != 0)
			{
				i2c->DAT = context->Address << 1;	// transmit slave+W
				instance->Ptr = context->Cmd;
			}
			else if (instance->WriteRem != 0)
			{
				i2c->DAT = context->Address << 1;	// transmit slave+W
				instance->Ptr = context->Data;
			}
			else
			{
				i2c->DAT = (context->Address << 1) | 1;	// transmit slave+R
				instance->Ptr = context->Data;
			}
			i2c->CONCLR = I2C_CON_START;
			break;
		case 0x18:	// slave+W transmitted, ack received
		case 0x28:	// data transmitted, ack received
			if (instance->CmdRem != 0)
			{
				i2c->DAT = *instance->Ptr++;
				if (--instance->CmdRem == 0)
					instance->Ptr = context->Data;
			}
			else if (instance->WriteRem != 0)
			{
				i2c->DAT = *instance->Ptr++;
				instance->WriteRem--;
			}
			else if (instance->ReadRem != 0)
			{
				i2c->CONSET = I2C_CON_START;	// transmit restart
			}
			else
			{
				i2c->CONSET = I2C_CON_STOP;		// transmit stop

				context->Error = I2C_OK;
				exos_event_set(&context->Done);
			}
			break;
		case 0x20:	// slave+W transmitted, nack received
			i2c->CONCLR = I2C_CON_START;
			i2c->CONSET = I2C_CON_STOP;	// transmit stop

			context->Error = I2C_NACK;
			exos_event_set(&context->Done);
			break;
		case 0x30:	// data transmitted, nack received
			i2c->CONSET = I2C_CON_STOP;

			context->Error = I2C_NACK;
			exos_event_set(&context->Done);
			break;
		case 0x38:	// arbitration lost during start
			i2c->CONCLR = I2C_CON_START;

			context->Error = I2C_BUS_ERROR;
			exos_event_set(&context->Done);
			break;
		
		case 0x40:	// slave+R transmitted, ack received
			if (--instance->ReadRem == 0) 
				i2c->CONCLR = I2C_CON_AA;	// transmit nack with 1st byte
			else
				i2c->CONSET = I2C_CON_AA;	// transmit ack with 1st byte
			break;
		case 0x50:	// data received, ack transmitted
			*instance->Ptr++ = i2c->DAT;
			ASSERT(instance->ReadRem != 0, KERNEL_ERROR_KERNEL_PANIC);
			if (--instance->ReadRem == 0)
				i2c->CONCLR = I2C_CON_AA;	// transmit nack with next byte
			break;
		case 0x58:	// data received, nack transmitted
			*instance->Ptr++ = i2c->DAT;
			ASSERT(instance->ReadRem == 0, KERNEL_ERROR_KERNEL_PANIC);
			i2c->CONSET = I2C_CON_STOP;

			context->Error = I2C_OK;
			exos_event_set(&context->Done);
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
			break;
	}
	i2c->CONCLR = I2C_CON_SI;	// clear int flag and continue
}

void I2C0_IRQHandler()
{
	_irq(0);
}

void I2C1_IRQHandler()
{
	_irq(1);
}

void I2C2_IRQHandler()
{
	_irq(2);
}

static void _transmit(unsigned module, i2c_context_t *context, unsigned write_len, unsigned read_len)
{
	i2c_instance_t *instance;
	i2c_module_t *i2c = _get_module(module, &instance);
	exos_mutex_lock(&instance->Lock);
	instance->Context = context;
	instance->WriteRem = write_len;
	instance->ReadRem = read_len;
	instance->CmdRem = context->CmdLength;
	exos_event_create(&context->Done, EXOS_EVENTF_NONE);
	i2c->CONSET = I2C_CON_START;
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

