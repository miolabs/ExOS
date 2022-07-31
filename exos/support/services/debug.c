#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <kernel/panic.h>
#include <support/services/init.h>
#include <comm/comm.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_SW_DRIVER, _register);

static EXOS_MUTEX _lock;
static EXOS_IO_ENTRY *_io = NULL;
static unsigned char _buffer[256];

static void _register()
{
	static COMM_IO_ENTRY comm;
#ifdef EXOS_DEBUG_PATH
	if (comm_io_create_from_path(&comm, EXOS_DEBUG_PATH, EXOS_IOF_WAIT))
	{
		unsigned long baudrate = 115200;
		comm_io_set_attr(&comm, COMM_ATTR_BAUDRATE, &baudrate);
		if (0 == comm_io_open(&comm))
		{
#ifdef DEBUG
			comm_io_get_attr(&comm, COMM_ATTR_BAUDRATE, &baudrate);
#endif
			_io = (EXOS_IO_ENTRY *)&comm;
		}
	}
#endif

	exos_mutex_create(&_lock);
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
		exos_mutex_lock(&_lock);
		int done = vsprintf(_buffer, format, args);
		if (done >= sizeof(_buffer))
			kernel_panic(KERNEL_ERROR_MEMORY_CORRUPT);

		done = debug_print(_buffer, done);
		exos_mutex_unlock(&_lock);
		return done;
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







