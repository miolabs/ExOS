#include "comm_handler.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "handler", __VA_ARGS__)

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	.Read = _read, .Write = _write };

void handler_create_context(handler_context_t *hc, const char *name, io_buffer_t *input, io_buffer_t *output)
{
	ASSERT(hc != NULL, KERNEL_ERROR_NULL_POINTER);
	hc->Input = input;
	hc->Output = output;
	hc->Entry = NULL;
	exos_event_create(&hc->OpenEvent, EXOS_EVENTF_AUTORESET);
	exos_io_add_device(&hc->Device, name, &_io_driver, hc);	
}


static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	handler_context_t *hc = io->DriverContext;
	ASSERT(hc != NULL, KERNEL_ERROR_NULL_POINTER);

	if (hc->Entry == NULL)
	{
		_verbose(VERBOSE_COMMENT, "file opened");
		
		io_buffer_t *input = hc->Input;
		if (input != NULL)
			exos_io_buffer_create(input, input->Buffer, input->Size, &io->InputEvent, NULL);

		io_buffer_t *output = hc->Output;
		if (output != NULL)
			exos_io_buffer_create(output, output->Buffer, output->Size, NULL, &io->OutputEvent);

		hc->Entry = io;
		exos_event_set(&hc->OpenEvent);
		return IO_OK;
	}
	_verbose(VERBOSE_ERROR, "file open failed (already open)");
	return IO_ERROR_ALREADY_LOCKED;
}

static void _close(io_entry_t *io)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	handler_context_t *hc = io->DriverContext;
	ASSERT(hc != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io == hc->Entry, KERNEL_ERROR_KERNEL_PANIC);

	_verbose(VERBOSE_COMMENT, "file closed");
	hc->Entry = NULL;
	exos_event_set(&hc->OpenEvent);
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	handler_context_t *hc = io->DriverContext;
	ASSERT(hc != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (io == hc->Entry && hc->Input != NULL)
	{
		int done = exos_io_buffer_read(hc->Input, buffer, length);
		return done;
	}
	return -1;
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	handler_context_t *hc = io->DriverContext;
	ASSERT(hc != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (io == hc->Entry && hc->Output != NULL)
	{
		int done = exos_io_buffer_write(hc->Output, buffer, length);
		return done;
	}
	return -1;
}





