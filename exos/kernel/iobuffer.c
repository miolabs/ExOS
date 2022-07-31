#include "iobuffer.h"
#include <kernel/panic.h>

void exos_io_buffer_create(io_buffer_t *iobuf, unsigned char *buffer, unsigned short size, event_t *not_empty, event_t *not_full)
{
	ASSERT(iobuf != NULL && buffer != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(size != 0, KERNEL_ERROR_KERNEL_PANIC);

	*iobuf = (io_buffer_t) { .Buffer = buffer, .Size = size }; 
	iobuf->NotEmptyEvent = not_empty;
	iobuf->NotFullEvent = not_full;
}


unsigned exos_io_buffer_write(io_buffer_t *iobuf, const unsigned char *buffer, unsigned length)
{
	ASSERT(iobuf != NULL && buffer != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned done;
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

	if (done != 0)
	{
#ifdef EXOS_OLD
		if (iobuf->EmptyEvent != NULL)
			exos_event_reset(iobuf->EmptyEvent);
#endif
		if (iobuf->NotEmptyEvent)
			exos_event_set(iobuf->NotEmptyEvent);
	}
	return done;
}

unsigned exos_io_buffer_read(io_buffer_t *iobuf, unsigned char *buffer, unsigned length)
{
	ASSERT(iobuf != NULL && buffer != NULL, KERNEL_ERROR_NULL_POINTER);

	int done;
	for(done = 0; done < length; done++)
	{
		int index = iobuf->ConsumeIndex;
		if (index == iobuf->ProduceIndex) 
		{
			if (iobuf->NotEmptyEvent)
				exos_event_reset(iobuf->NotEmptyEvent);
#ifdef EXOS_OLD
			if (iobuf->EmptyEvent != NULL)
				exos_event_set(iobuf->EmptyEvent);
#endif
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

unsigned exos_io_buffer_avail(io_buffer_t *iobuf)
{
	ASSERT(iobuf != NULL, KERNEL_ERROR_NULL_POINTER);

	int avail = iobuf->ProduceIndex - iobuf->ConsumeIndex;
	if (avail < 0) avail += iobuf->Size; 
	return avail;
}

unsigned exos_io_buffer_discard(io_buffer_t *iobuf, unsigned length)
{
	ASSERT(iobuf != NULL, KERNEL_ERROR_NULL_POINTER);

	if (length != 0)
	{
		unsigned avail = exos_io_buffer_avail(iobuf);
		if (length > avail) length = avail;
	
		unsigned index = iobuf->ConsumeIndex + length;
		if (index >= iobuf->Size) index -= iobuf->Size;
		iobuf->ConsumeIndex = index;

		if (iobuf->NotFullEvent != NULL)
			exos_event_set(iobuf->NotFullEvent);
	}
	return length;
}

unsigned exos_io_buffer_peek(io_buffer_t *iobuf, unsigned offset, void **pptr)
{
#ifdef DEBUG
	if (iobuf == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
#endif

	unsigned avail = exos_io_buffer_avail(iobuf);
	if (offset >= avail) return 0;

	unsigned index = iobuf->ConsumeIndex + offset;
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


