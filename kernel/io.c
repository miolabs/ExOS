#include "io.h"
#include <kernel/panic.h>
#include <kernel/tree.h>
#include <kernel/event.h>
#include <comm/comm.h>

#ifndef EXOS_NO_NET
#include <net/net.h>
#endif

void __io_initialize()
{
	__tree_initialize();
	comm_initialize();
#ifndef EXOS_NO_NET
	net_initialize();
#endif
}

void exos_io_create(EXOS_IO_ENTRY *io, EXOS_IO_TYPE type, const EXOS_IO_DRIVER *driver, EXOS_IO_FLAGS flags)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	*io = (EXOS_IO_ENTRY) {
#ifdef DEBUG
		.Node = (EXOS_NODE) { .Type = EXOS_NODE_IO_ENTRY },
#endif
		.Type = type, .Driver = driver, .Flags = flags };
	exos_event_create(&io->InputEvent);
	exos_event_create(&io->OutputEvent);
}

void exos_io_set_flags(EXOS_IO_ENTRY *io, EXOS_IO_FLAGS flags)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	io->Flags = flags;
}

void exos_io_set_timeout(EXOS_IO_ENTRY *io, unsigned long timeout)
{
#ifdef DEBUG
	if (io == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	io->Timeout = timeout;
}

int exos_io_read(EXOS_IO_ENTRY *io, void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (io->Flags & EXOS_IOF_WAIT)
		exos_event_wait(&io->InputEvent, io->Timeout);

	const EXOS_IO_DRIVER *driver = io->Driver;
	return driver->Read(io, buffer, length);
}

int exos_io_write(EXOS_IO_ENTRY *io, const void *buffer, unsigned long length)
{
#ifdef DEBUG
	if (io == NULL || io->Driver == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	const EXOS_IO_DRIVER *driver = io->Driver;
	if (io->Flags & EXOS_IOF_WAIT)
	{
		int done = 0;
		while(length > 0)
		{
			exos_event_wait(&io->OutputEvent, io->Timeout);
	
			int done2 = driver->Write(io, buffer + done, length);
			if (done2 < 0) return -1;
			if (done2 == 0)
				break;

			done += done2;
			length -= done2;
		}
		return done;
	}
	else
	{
		return driver->Write(io, buffer, length);
	}
}

void exos_io_buffer_create(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short size)
{
#ifdef DEBUG
	if (iobuf == NULL || buffer == NULL || size == 0)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	*iobuf = (EXOS_IO_BUFFER) { .Buffer = buffer, .Size = size }; 
}


int exos_io_buffer_write(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short length)
{
#ifdef DEBUG
	if (iobuf == NULL || buffer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int done;
	for(done = 0; done < length; done++)
	{
		int index = iobuf->ProduceIndex + 1;
        if (index == iobuf->Size) index = 0;
		if (index == iobuf->ConsumeIndex)
		{
			if (iobuf->NotFullEvent != NULL)
				exos_event_reset(iobuf->NotFullEvent);
			break;
		}
		iobuf->Buffer[iobuf->ProduceIndex] = ((unsigned char *)buffer)[done];
		iobuf->ProduceIndex = index;
	}

	if (done == length && iobuf->NotFullEvent != NULL)
	{
		int index = iobuf->ProduceIndex + 1;
		if (index == iobuf->Size) index = 0;
		if (index == iobuf->ConsumeIndex)
		{
			exos_event_reset(iobuf->NotFullEvent);
		}
	}

	if (done != 0 && iobuf->NotEmptyEvent)
		exos_event_set(iobuf->NotEmptyEvent);
	return done;
}

int exos_io_buffer_read(EXOS_IO_BUFFER *iobuf, void *buffer, unsigned short length)
{
#ifdef DEBUG
	if (iobuf == NULL || buffer == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int done;
	for(done = 0; done < length; done++)
	{
		int index = iobuf->ConsumeIndex;
		if (index == iobuf->ProduceIndex) 
		{
			if (iobuf->NotEmptyEvent)
				exos_event_reset(iobuf->NotEmptyEvent);
			break;
		}
		((unsigned char *)buffer)[done] = iobuf->Buffer[index++];
		if (index == iobuf->Size) index = 0;
		iobuf->ConsumeIndex = index;
	}

	if (done != 0 && iobuf->NotFullEvent != NULL)
		exos_event_set(iobuf->NotFullEvent);
	return done;
}

int exos_io_buffer_avail(EXOS_IO_BUFFER *iobuf)
{
#ifdef DEBUG
	if (iobuf == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int avail = iobuf->ProduceIndex - iobuf->ConsumeIndex;
	if (avail < 0) avail += iobuf->Size; 
	return avail;
}

int exos_io_buffer_discard(EXOS_IO_BUFFER *iobuf, unsigned short length)
{
#ifdef DEBUG
	if (iobuf == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	if (length != 0)
	{
		int avail = exos_io_buffer_avail(iobuf);
		if (length > avail) length = avail;
	
		int index = iobuf->ConsumeIndex + length;
		if (index >= iobuf->Size) index -= iobuf->Size;
		iobuf->ConsumeIndex = index;

		if (iobuf->NotFullEvent != NULL)
			exos_event_set(iobuf->NotFullEvent);
	}
	return length;
}

int exos_io_buffer_peek(EXOS_IO_BUFFER *iobuf, unsigned short offset, void **pptr)
{
#ifdef DEBUG
	if (iobuf == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	int avail = exos_io_buffer_avail(iobuf);
	if (offset >= avail) return 0;

	int index = iobuf->ConsumeIndex + offset;
	if (index < iobuf->Size)
	{
		if (pptr) *pptr = iobuf->Buffer + index;
		return ((index + avail - offset) < iobuf->Size) ? 
			(avail - offset) : (iobuf->Size - index);
	}
	else
	{
		index -= iobuf->Size;
		if (pptr) *pptr = iobuf->Buffer + index;
		return avail - offset;
	}
}

