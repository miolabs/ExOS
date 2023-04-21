#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include <kernel/io.h>
#include <kernel/iobuffer.h>

typedef struct
{
	io_tree_device_t Device;
	io_entry_t *Entry;
	void *Buffer;
	unsigned BufferSize;
	io_buffer_t Iob;
} input_device_context_t;

void input_create_context(input_device_context_t *context, const char *name, void *buffer, unsigned buffer_size);

#endif // INPUT_DEVICE_H


