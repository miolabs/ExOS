#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <kernel/panic.h>
#include <support/services/init.h>
#include <comm/comm.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_SW_DRIVER, _register);

static COMM_IO_ENTRY _comm;
static EXOS_IO_ENTRY *_io = NULL;
static unsigned char _buffer[256];

static void _register()
{
#ifdef EXOS_DEBUG_PATH
	EXOS_TREE_NODE *node = exos_tree_find_path(NULL, EXOS_DEBUG_PATH);
	if (node != NULL && node->Type == EXOS_TREE_NODE_DEVICE)
	{
		EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)node;
		comm_io_create(&_comm, dev_node->Device, dev_node->Unit, EXOS_IOF_WAIT);
		unsigned long baudrate = 115200;
		comm_io_set_attr(&_comm, COMM_ATTR_BAUDRATE, &baudrate);
		if (0 == comm_io_open(&_comm))
		{
#ifdef DEBUG
			comm_io_get_attr(&_comm, COMM_ATTR_BAUDRATE, &baudrate);
#endif
			_io = (EXOS_IO_ENTRY *)&_comm;
		}
	}
#endif
}

int debug_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	return debug_vprintf(format, args);
}

int debug_vprintf(const char *format, va_list args)
{
	if (_io != NULL)
	{
		int length = vsprintf(_buffer, format, args);
		if (length >= sizeof(_buffer))
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

		return debug_print(_buffer, length);
	}
	return -1;
}

int debug_print(const char *buffer, int length)
{
	if (_io != NULL)
	{
		int done = exos_io_write(_io, buffer, length);
#ifdef EXOS_DEBUG_SYNC
		exos_io_sync(_io);
#endif
		return done;
	}
	return -1;
}







