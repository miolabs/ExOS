#ifndef HAL_I2C_HAL_H
#define HAL_I2C_HAL_H

#include <kernel/event.h>
#include <kernel/panic.h>
#include <stdbool.h>

typedef enum
{
	I2C_OK = 0,
	I2C_NACK,
	I2C_BUS_ERROR,
} i2c_error_t;

typedef struct
{
	event_t Done;
	unsigned char *Cmd;
	unsigned char *Data;
	unsigned char Address;
	unsigned char CmdLength;
	unsigned char Length;
	i2c_error_t Error;
} i2c_context_t;

#define HAL_I2C_GENERAL_CALL_ADDR 0

static void inline hal_i2c_init_context(i2c_context_t *context, unsigned char addr, void *cmd, unsigned cmd_len)
{
	ASSERT(context != nullptr, KERNEL_ERROR_NULL_POINTER);
	context->Cmd = cmd;
	context->Address = addr;
	context->CmdLength = cmd_len;
	context->Length = context->Error = 0;
	exos_event_create(&context->Done, EXOS_EVENTF_AUTORESET);
}

// prototypes
void hal_i2c_initialize(unsigned module, unsigned bitrate);
bool hal_i2c_write(unsigned module, i2c_context_t *context, const void *data, unsigned length);
bool hal_i2c_read(unsigned module, i2c_context_t *context, void *data, unsigned length);

#endif // HAL_I2C_HAL_H

