// io driver for uart_hal 
// by Miguel Fides

#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <support/uart_hal.h>
#include <support/services/init.h>
#include <stdio.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_IO_DRIVER, _register);

#ifndef UART_BUFFER_SIZE 
#define UART_BUFFER_SIZE 64
#endif
#ifndef UART_MODULE_COUNT
#define UART_MODULE_COUNT 1
#endif

typedef char uart_device_name_t[8];

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
//static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
//static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _comm_driver = {
	.Open = _open, .Close = _close,
//	.GetAttr = _get_attr, .SetAttr = _set_attr, 
	.Read = _read, .Write = _write };

static io_tree_device_t _devices[UART_MODULE_COUNT];
static uart_control_block_t _cb[UART_MODULE_COUNT];
static uart_device_name_t _dev_name[UART_MODULE_COUNT];
static mutex_t _lock;

static void _register()
{
	exos_mutex_create(&_lock);

	for(int i = 0; i < UART_MODULE_COUNT; i++)
	{
		_cb[i] = (uart_control_block_t) { .Baudrate = 9600 };
		sprintf(_dev_name[i], "uart%d", i);
		exos_io_add_device(&_devices[i], _dev_name[i], &_comm_driver, &_cb[i]);
		_devices[i].Port = i;
	}
}

static bool _alloc_uart_buffers(uart_control_block_t *cb, io_entry_t *io)
{
	if (cb->Input.Buffer != NULL || cb->Output.Buffer != NULL) return -1;

	void *buffer = exos_mem_alloc(UART_BUFFER_SIZE * 2, EXOS_MEMF_ANY);
	if (buffer != NULL)
	{
		exos_io_buffer_create(&cb->Input, buffer, UART_BUFFER_SIZE, &io->InputEvent, NULL);
		exos_io_buffer_create(&cb->Output, buffer + UART_BUFFER_SIZE, UART_BUFFER_SIZE, NULL, &io->OutputEvent);
		return true;
	}
	return false;
}

static void _free_uart_buffers(uart_control_block_t *cb)
{
	if (cb->Input.Buffer != NULL)
	{
		void *ptr = cb->Input.Buffer;
		exos_mem_free(ptr);

		cb->Input.Buffer = NULL;
		cb->Output.Buffer = NULL;
	}
}

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	ASSERT(io->Port < UART_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	uart_control_block_t *cb = (uart_control_block_t *)io->DriverContext;
	ASSERT(cb != NULL, KERNEL_ERROR_NULL_POINTER);

	bool done = false;
	exos_mutex_lock(&_lock);

	if (_alloc_uart_buffers(cb, io))
	{
		//cb->Handler = _handler;
		//cb->HandlerState = io;
		if (cb->Baudrate == 0) cb->Baudrate = 9600;
		if (uart_initialize(io->Port, cb))
		{
			exos_event_set(&io->OutputEvent);
			done = 1;
		}
		_free_uart_buffers(cb);
	}
	exos_mutex_unlock(&_lock);
	return done ? IO_OK : IO_ERROR_IO_ERROR;
}

//static int _get_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
//{
//	int done = 0;
//	if (io->Port < UART_MODULE_COUNT)
//	{
//		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
//		switch(attr)
//		{
//			case COMM_ATTR_BAUDRATE:
//				*(unsigned long *)value = cb->Baudrate;
//				done = 1;
//				break;
//		}
//	}
//	return done ? 0 : -1;
//}

//static UART_MODE _uart_mode(COMM_RS485_CONFIG *config)
//{
//	// TODO
//}

//static int _set_attr(COMM_IO_ENTRY *io, COMM_ATTR_ID attr, void *value)
//{
//	int done = 0;
//	if (io->Port < UART_MODULE_COUNT)
//	{
//		exos_mutex_lock(&_lock);
//
//		UART_CONTROL_BLOCK *cb = &_cb[io->Port];
//		switch(attr)
//		{
//			case COMM_ATTR_BAUDRATE:
//				if (cb->Handler != NULL)
//				{
//					uart_disable(io->Port);
//					cb->Baudrate = *(unsigned long *)value;
//					uart_initialize(io->Port, cb);
//				}
//				else
//				{
//					cb->Baudrate = *(unsigned long *)value;
//				}
//				done = 1;
//				break;
////			case COMM_ATTR_RS485_CONFIG:
////				uart_set_rs485(io->Port,
////					_uart_mode((COMM_RS485_CONFIG *)value),
////						((COMM_RS485_CONFIG *)value)->DirectionDisableDelay,
////						((COMM_RS485_CONFIG *)value)->Address);
////				done = 1;
////				break;
//		}
//        exos_mutex_unlock(&_lock);
//	}
//	return done ? 0 : -1;
//}


static void _close(io_entry_t *io)
{
	if (io->Port < UART_MODULE_COUNT)
	{
		exos_mutex_lock(&_lock);
		uart_control_block_t *cb = &_cb[io->Port];

		uart_disable(io->Port);
		_free_uart_buffers(cb);

        exos_mutex_unlock(&_lock);
	}
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	return uart_read(io->Port, buffer, length);
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
//	if (length != 0) 
//		exos_event_reset(&io->SyncEvent);

	return uart_write(io->Port, buffer, length);
}



