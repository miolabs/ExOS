#ifndef PIPE_DEVICE_H
#define PIPE_DEVICE_H

#include <kernel/io.h>
#include <kernel/iobuffer.h>

typedef struct
{
	io_tree_device_t Device;
	io_entry_t *Entry;
	io_buffer_t *Input;	// for read()
	io_buffer_t *Output; // for write()
} pipe_context_t;

void pipe_create_context(pipe_context_t *context, const char *name, io_buffer_t *input, io_buffer_t *output);

#endif // PIPE_DEVICE_H


