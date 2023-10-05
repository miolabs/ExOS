#include "comm_pipe.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>

#define _verbose(level, ...) verbose(level, "pipe", __VA_ARGS__)

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length);

static const io_driver_t _io_driver = {
	.Open = _open, .Close = _close,
	.Read = _read, .Write = _write };

void pipe_create_context(pipe_context_t *context, const char *name, io_buffer_t *input, io_buffer_t *output)
{
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	context->Input = input;
	context->Output = output;
	context->Entry = NULL;
	exos_event_create(&context->OpenEvent, EXOS_EVENTF_AUTORESET);
	exos_io_add_device(&context->Device, name, &_io_driver, context);	
}


static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	pipe_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);

	if (context->Entry == NULL)
	{
		_verbose(VERBOSE_COMMENT, "file opened");
		
		io_buffer_t *input = context->Input;
		if (input != NULL)
			exos_io_buffer_create(input, input->Buffer, input->Size, &io->InputEvent, NULL);

		io_buffer_t *output = context->Output;
		if (output != NULL)
			exos_io_buffer_create(output, output->Buffer, output->Size, NULL, &io->OutputEvent);

		context->Entry = io;
		exos_event_set(&context->OpenEvent);
		return IO_OK;
	}
	_verbose(VERBOSE_ERROR, "file open failed (already open)");
	return IO_ERROR_ALREADY_LOCKED;
}

static void _close(io_entry_t *io)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	pipe_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io == context->Entry, KERNEL_ERROR_KERNEL_PANIC);

	_verbose(VERBOSE_COMMENT, "file closed");
	context->Entry = NULL;
	exos_event_set(&context->OpenEvent);
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned length)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	pipe_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (io == context->Entry && context->Input != NULL)
	{
		int done = exos_io_buffer_read(context->Input, buffer, length);
		return done;
	}
	return -1;
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned length)
{
	ASSERT(io != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(io->Driver == &_io_driver, KERNEL_ERROR_KERNEL_PANIC);
	pipe_context_t *context = io->DriverContext;
	ASSERT(context != NULL, KERNEL_ERROR_NULL_POINTER);
	
	if (io == context->Entry && context->Output != NULL)
	{
		int done = exos_io_buffer_write(context->Output, buffer, length);
		return done;
	}
	return -1;
}





