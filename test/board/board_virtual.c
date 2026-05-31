// Dummy "virtual board" for the host port.
//
// Mounts a single console device at /dev/console whose Write callback funnels
// bytes to the real host stdout (via __host_console_write in the host machine
// layer). Build the host with -DSTDOUT_STREAM="/dev/console" so the POSIX stdio
// layer opens this device and printf()/puts() reach the terminal.
//
// LEDs and discrete outputs have no hardware to drive, so they are no-ops.
//
// Compiled WITH exos/include (bool == int), so the io_driver_t ABI matches the
// rest of the kernel.

#include "board.h"
#include <kernel/io.h>
#include <kernel/machine/hal.h>
#include <support/services/init.h>
#include <stdio.h>
#include <stdarg.h>

// Defined in the host machine layer (machine_host.c), which is compiled against
// the system headers so it can reach the kernel's real fd 1 without colliding
// with exos's own write().
extern long __host_console_write(const void *buf, unsigned long len);

static io_error_t _console_open(io_entry_t *io, const char *path, io_flags_t flags)
{
	(void)io; (void)path; (void)flags;
	return IO_OK;
}

static void _console_close(io_entry_t *io)
{
	(void)io;
}

static int _console_read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	(void)io; (void)buffer; (void)length;
	return 0;							// no console input on host
}

static int _console_write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	(void)io;
	return (int)__host_console_write(buffer, length);
}

static int _console_sync(io_entry_t *io)
{
	(void)io;
	return 0;
}

static const io_driver_t _console_driver = {
	.Open = _console_open,
	.Close = _console_close,
	.Read = _console_read,
	.Write = _console_write,
	.Sync = _console_sync,
};

static io_tree_device_t _console_device;

// Mount the console under /dev. This must run after __io_initialize() has built
// the device tree but before the POSIX stdio layer opens /dev/console, so it is
// registered as an IO-driver initializer (higher priority than EXOS_INIT_POSIX)
// rather than from __board_init (which runs before the tree exists).
static void _register_console(void)
{
	exos_io_add_device(&_console_device, "console", &_console_driver, NULL);
}
EXOS_INITIALIZER(_console_init, EXOS_INIT_IO_DRIVER, _register_console);

void __board_init()
{
}

// On a real board this hands memory regions to the allocator; the host machine
// layer already registers its heap via the linker-symbol aliases, so nothing
// to add here.
void __board_add_memory()
{
}

void board_led_set(board_led_t led, bool state)
{
	(void)led; (void)state;
}

void board_output(board_output_t mask, board_output_t value)
{
	(void)mask; (void)value;
}

// Diagnostic output. On a real board this goes to a dedicated debug UART/RTT;
// on the host we route it to the same console (stdout). vprintf no-ops while
// stdout is unset (early boot), which is fine for the diagnostic call sites.
int debug_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int r = vprintf(format, args);
	va_end(args);
	return r;
}
