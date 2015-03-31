// LPC2K comm driver for built-in uarts
// by Miguel Fides

#include "comm_lpc2k.h"
#include <kernel/memory.h>
#include <support/lpc2k/uart.h>
#include <support/services/init.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_IO_DRIVER, _register);

#ifndef UART_BUFFER_SIZE 
#define UART_BUFFER_SIZE 64
#endif

static int _open(COMM_IO_ENTRY *io);
static void _close(COMM_IO_ENTRY *io);
static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length);
static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length);

static const COMM_DRIVER _comm_driver = {
	.Open = _open, .Close = _close,
    .GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };

static COMM_DEVICE _comm_device = { .Driver = &_comm_driver, .PortCount = 4 };
static EXOS_TREE_DEVICE _devices[] = {
	{ .Name = "comm0", .Device = &_comm_device, .Unit = 0 },
	{ .Name = "comm1", .Device = &_comm_device, .Unit = 1 },
	{ .Name = "comm2", .Device = &_comm_device, .Unit = 2 },
	{ .Name = "comm3", .Device = &_comm_device, .Unit = 3 } };

static void _handler(UART_EVENT event, void *state);
static UART_CONTROL_BLOCK _cb[UART_MODULE_COUNT];
static EXOS_MUTEX _lock;

static void _register()
{
	exos_mutex_create(&_lock);

	for(int i = 0; i < _comm_device.PortCount; i++)
	{
		_cb[i] = (UART_CONTROL_BLOCK) { .Baudrate = 9600 };
		EXOS_TREE_DEVICE *node = &_devices[i];
		comm_add_device(node, "dev");
	}
}

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
	int done = 0;
	if (io->Port < UART_MODULE_COUNT)
	{
		exos_mutex_lock(&_lock);

		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
		if (_alloc_uart_buffers(cb) == 0)
		{
			cb->Handler = _handler;
			cb->HandlerState = io;
			if (cb->Baudrate == 0) cb->Baudrate = 9600;
			uart_initialize(io->Port, cb);

			exos_event_set(&io->OutputEvent);
			
			done = 1;
		}
		exos_mutex_unlock(&_lock);
	}
	return done ? 0 : -1;
}

static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	int done = 0;
	if (io->Port < UART_MODULE_COUNT)
	{
		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
		switch(attr)
		{
			case COMM_ATTR_BAUDRATE:
				*(unsigned long *)value = cb->Baudrate;
				done = 1;
				break;
		}
	}
	return done ? 0 : -1;
}

static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
{
	int done = 0;
	if (io->Port < UART_MODULE_COUNT)
	{
		exos_mutex_lock(&_lock);

		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
		switch(attr)
		{
			case COMM_ATTR_BAUDRATE:
				if (cb->Handler != NULL)
				{
					uart_disable(io->Port);
					cb->Baudrate = *(unsigned long *)value;
					uart_initialize(io->Port, cb);
				}
				else
				{
					cb->Baudrate = *(unsigned long *)value;
				}
				done = 1;
				break;
		}
        exos_mutex_unlock(&_lock);
	}
	return done ? 0 : -1;
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
		exos_mutex_lock(&_lock);
		UART_CONTROL_BLOCK *cb = &_cb[io->Port];

		uart_disable(io->Port);
		_free_uart_buffers(cb);

        exos_mutex_unlock(&_lock);
	}
}

static int _read(COMM_IO_ENTRY *io, unsigned char *buffer, unsigned long length)
{
	return uart_read(io->Port, buffer, length);
}

static int _write(COMM_IO_ENTRY *io, const unsigned char *buffer, unsigned long length)
{
	exos_event_reset(&io->SyncEvent);

	return uart_write(io->Port, buffer, length);
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
		case UART_EVENT_OUTPUT_READY:
			exos_event_set(&io->OutputEvent);
			break;
		case UART_EVENT_OUTPUT_FULL:
			exos_event_reset(&io->OutputEvent);
			break;
		case UART_EVENT_OUTPUT_EMPTY:
			exos_event_set(&io->SyncEvent);
			break;
	}
}





