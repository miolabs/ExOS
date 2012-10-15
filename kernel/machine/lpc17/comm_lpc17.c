// LPC17xx MAC Driver
// by Miguel Fides

#include "comm_lpc17.h"
#include <kernel/memory.h>
#include <support/lpc17/uart.h>
#include <support/board_hal.h>

#ifndef UART_BUFFER_SIZE 
#define UART_BUFFER_SIZE 64
#endif

static void _handler(UART_EVENT event, void *state);
static UART_CONTROL_BLOCK _cb[UART_MODULE_COUNT];

static int _open(COMM_IO_ENTRY *io);
static int _set_attrs(COMM_IO_ENTRY *io);
static void _close(COMM_IO_ENTRY *io);
static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
static int _write(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);

const COMM_DRIVER __comm_driver_lpc17 = {
	.Open = _open, .SetAttrs = _set_attrs, .Close = _close,
	.Read = _read, .Write = _write };

static int _alloc_uart_buffers(UART_CONTROL_BLOCK *cb)
{
	if (cb->InputBuffer.Buffer != NULL || cb->OutputBuffer.Buffer != NULL) return -1;

	void *buffer = exos_mem_alloc(UART_BUFFER_SIZE * 2, EXOS_MEMF_ANY);
	if (buffer != NULL)
	{
		cb->InputBuffer = (UART_BUFFER) { .Buffer = buffer, .Size = UART_BUFFER_SIZE };
		cb->OutputBuffer = (UART_BUFFER) { .Buffer = buffer + UART_BUFFER_SIZE, .Size = UART_BUFFER_SIZE };
		return 0;
	}
	return -1;
}

static int _open(COMM_IO_ENTRY *io)
{
	if (io->Port < UART_MODULE_COUNT)
	{
		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
		if (_alloc_uart_buffers(cb) == 0)
		{
			cb->Handler = _handler;
			cb->HandlerState = io;
			hal_board_init_pinmux(HAL_RESOURCE_UART, io->Port);
			
			if (io->Baudrate == 0) io->Baudrate = 9600;
			uart_initialize(io->Port, io->Baudrate, cb);
			return 0;
		}
	}
	return -1;
}

static int _set_attrs(COMM_IO_ENTRY *io)
{
	uart_set_baudrate(io->Port, io->Baudrate);
	return 0;
}

static void _free_uart_buffers(UART_CONTROL_BLOCK *cb)
{
	if (cb->InputBuffer.Buffer != NULL)
	{
		void *ptr = cb->InputBuffer.Buffer;
		exos_mem_free(ptr);

		cb->InputBuffer.Buffer = NULL;
		cb->OutputBuffer.Buffer = NULL;
	}
}

static void _close(COMM_IO_ENTRY *io)
{
	if (io->Port < UART_MODULE_COUNT)
	{
		UART_CONTROL_BLOCK *cb = &_cb[io->Port];

		uart_disable(io->Port);
		_free_uart_buffers(cb);
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	return uart_read(io->Port, buffer, length);
}

static int _write(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	return -1;
}

static void _handler(UART_EVENT event, void *state)
{
	COMM_IO_ENTRY *io = (COMM_IO_ENTRY *)state;
	switch(event)
	{
		case UART_EVENT_INPUT_READY:
			exos_event_set(&io->InputEvent);
			break;
		case UART_EVENT_INPUT_EMPTY:
			exos_event_reset(&io->InputEvent);
			break;
	}
}





