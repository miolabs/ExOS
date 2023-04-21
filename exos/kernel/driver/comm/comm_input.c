#include "comm_input.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "input", __VA_ARGS__)

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	.Read = _read, .Write = _write };

void input_create_context(input_device_context_t *context, const char *name, void *buffer, unsigned buffer_size)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	context->Buffer = buffer;
	context->BufferSize = buffer_size;
	context->Entry = NULL;
	exos_io_add_device(&context->Device, name, &_io_driver, context);	
}


static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	input_device_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	if (context->Entry == NULL)
	{
		_verbose(VERBOSE_COMMENT, "file opened");
		exos_io_buffer_create(&context->Iob, context->Buffer, context->BufferSize, &io->InputEvent, NULL);
		context->Entry = io;
		return IO_OK;
	}
	_verbose(VERBOSE_ERROR, "file open failed (already open)");
	return IO_ERROR_ALREADY_LOCKED;
}

static void _close(io_entry_t *io)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	input_device_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io == context->Entry, KERNEL_ERROR_KERNEL_PANIC);

	_verbose(VERBOSE_COMMENT, "file closed");
	context->Entry = NULL;
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	input_device_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (io == context->Entry)
	{
		int done = exos_io_buffer_read(&context->Iob, buffer, length);
		return done;
	}
	return -1;
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	return -1;
}





