#include "comm_rtt.h"
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/panic.h>
#include <support/services/init.h>
#include <kernel/machine/hal.h>
#include <stdio.h>
#include <string.h>

static void _register();
EXOS_INITIALIZER(_init, EXOS_INIT_IO_DRIVER, _register);

#ifndef RTT_BUFFER_SIZE
#define RTT_BUFFER_SIZE 64
#endif

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags);
static void _close(io_entry_t *io);
static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length);
static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length);
static const io_driver_t _rtt_driver = { 
	.Read = _read, .Write = _write,
	.Open = _open, .Close = _close };

static struct _rtt_context
{
	rtt_control_block_t cb;
	rtt_buffer_t up;
	rtt_buffer_t down;
} _SEGGER_RTT;

static unsigned char _up_buffer[RTT_BUFFER_SIZE] __aligned(8);
static unsigned char _down_buffer[RTT_BUFFER_SIZE] __aligned(8);

static void _register()
{
	static io_tree_device_t _device;

	struct _rtt_context *context = &_SEGGER_RTT;
	strcpy((char *)context->cb.Id, "SEGGER RTT");
	context->cb.MaxDownBuffers = 1;
	context->cb.MaxUpBuffers = 1;
	rtt_buffer_create(&context->down, "rtt_down", _down_buffer, sizeof(_down_buffer));
	rtt_buffer_create(&context->up, "rtt_up", _up_buffer, sizeof(_up_buffer));

	exos_io_add_device(&_device, "rtt", &_rtt_driver, NULL);
}

static io_error_t _open(io_entry_t *io, const char *path, io_flags_t flags)
{
	if (io->DriverContext != nullptr)
		return IO_ERROR_ALREADY_LOCKED;
	
	exos_event_set(&io->OutputEvent);
	io->DriverContext = &_SEGGER_RTT;
	return IO_OK;
}

static void _close(io_entry_t *io)
{
	struct _rtt_context *rtt = (struct _rtt_context *)io->DriverContext;
	ASSERT(rtt != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(rtt == &_SEGGER_RTT, KERNEL_ERROR_KERNEL_PANIC);
	io->DriverContext = nullptr;
}

static int _read(io_entry_t *io, unsigned char *buffer, unsigned int length)
{
	struct _rtt_context *rtt = (struct _rtt_context *)io->DriverContext;
	ASSERT(rtt != NULL, KERNEL_ERROR_NULL_POINTER);
	return rtt_read(&rtt->down, buffer, length);
}

static int _write(io_entry_t *io, const unsigned char *buffer, unsigned int length)
{
	struct _rtt_context *rtt = (struct _rtt_context *)io->DriverContext;
	ASSERT(rtt != NULL, KERNEL_ERROR_NULL_POINTER);
	int done = 0;
	while(1)
	{
		unsigned part = rtt_write(&rtt->up, buffer + done, length - done);
		if (part == 0)
			break;
		done += part;
		if (done == length)
			break;
	}
	return done;
}


void rtt_buffer_create(rtt_buffer_t *rtt, const char *name, void *buf, unsigned size)
{
	*rtt = (rtt_buffer_t) { .Name = name,
		.BufStart = buf, .BufSize = size }; 
}

int rtt_read(rtt_buffer_t *rtt, void *buf, unsigned int length)
{
	ASSERT(rtt != nullptr && buf != nullptr, KERNEL_ERROR_NULL_POINTER);
	int avail = (rtt->WriteOffset - rtt->ReadOffset);
	if (avail < 0) avail += rtt->BufSize;
	ASSERT(avail >= 0 && avail < rtt->BufSize, KERNEL_ERROR_KERNEL_PANIC);
	if (avail == 0)
		return 0;

	if (length > avail)
		length = avail;

	unsigned offset = rtt->ReadOffset;
	for (unsigned i = 0; i < length; i++)
	{
		((char *)buf)[i] = rtt->BufStart[offset++];
		if (offset == rtt->BufSize)
			offset = 0;
	}
	__machine_dsb();
	rtt->ReadOffset = offset;
	__machine_dsb();
	return length;
}

int rtt_write(rtt_buffer_t *rtt, const void *buf, unsigned int length)
{
	ASSERT(rtt != nullptr && buf != nullptr, KERNEL_ERROR_NULL_POINTER);
	int avail = (rtt->ReadOffset - rtt->WriteOffset) - 1;
	if (avail < 0) avail += rtt->BufSize;
	ASSERT(avail >= 0 && avail < rtt->BufSize, KERNEL_ERROR_KERNEL_PANIC);
	if (length > avail)
		length = avail;
	
	unsigned offset = rtt->WriteOffset;
	for (unsigned i = 0; i < length; i++)
	{
		rtt->BufStart[offset++] = ((char *)buf)[i];
		if (offset == rtt->BufSize)
			offset = 0;
	}
	__machine_dsb();
	rtt->WriteOffset = offset;
	__machine_dsb();
	return length;
}

