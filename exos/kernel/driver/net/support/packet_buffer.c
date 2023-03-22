#include "packet_buffer.h"
#include <kernel/panic.h> 

#define NBP_MIN_SIZE (sizeof(net_packet_buffer_header_t) - 4)



void net_packet_buffer_initialize(net_packet_buffer_t *npb, void *data, unsigned size)
{
	ASSERT(npb != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(size >= NBP_MIN_SIZE, KERNEL_ERROR_KERNEL_PANIC);
	npb->Buffer = data;
	npb->Size = size;
	npb->InputIndex = npb->OutputIndex = 0;
	exos_event_create(&npb->Notify, EXOS_EVENTF_AUTORESET);
}

static inline unsigned _usage(net_packet_buffer_t *npb)
{
	int usage = npb->InputIndex - npb->OutputIndex;
	if (usage < 0) usage += npb->Size;
	return usage;
}

static inline unsigned _push(net_packet_buffer_t *npb, void *data, unsigned length)
{
	ASSERT(length <= net_packet_buffer_free(npb), KERNEL_ERROR_KERNEL_PANIC);
	unsigned chunk = npb->Size - npb->InputIndex;
	if (chunk > length) chunk = length;

	unsigned offset = npb->InputIndex;
	unsigned int *src = (unsigned *)data;
	for(unsigned i = 0; i < chunk; i += sizeof(unsigned)) 
	{
		*(unsigned *)(npb->Buffer + offset) = *src++; 
		offset += sizeof(unsigned);
	}
	npb->InputIndex = (offset < npb->Size) ? offset : 0;
	return chunk;
}

unsigned net_packet_buffer_free(net_packet_buffer_t *npb)
{
	ASSERT(npb != NULL, KERNEL_ERROR_NULL_POINTER);
	unsigned usage = _usage(npb);
	ASSERT(usage <= (npb->Size - 4), KERNEL_ERROR_KERNEL_PANIC);
	return (npb->Size - 4) - usage;
}

bool net_packet_buffer_push(net_packet_buffer_t *npb, void *data, unsigned short length, unsigned short flags)
{
	ASSERT(npb != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(data != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(length != 0, KERNEL_ERROR_NOT_SUPPORTED);

	unsigned space = net_packet_buffer_free(npb);
	if (space >= (length + sizeof(net_packet_buffer_header_t)))
	{
		net_packet_buffer_header_t hdr = { .Length = HTON16(length), .Flags = HTON16(flags) };
		unsigned hdr_len = _push(npb, &hdr, sizeof(net_packet_buffer_header_t));
		ASSERT(hdr_len == sizeof(net_packet_buffer_header_t), KERNEL_ERROR_KERNEL_PANIC);

		void *ptr = data;
		while(length != 0)
		{
			unsigned chunk = _push(npb, ptr, length);
			ASSERT(chunk != 0, KERNEL_ERROR_KERNEL_PANIC);
			ptr += chunk;
			length -= chunk;
		}
		ASSERT(length == 0, KERNEL_ERROR_KERNEL_PANIC);

		exos_event_set(&npb->Notify);
		return true;
	}
	return false;
}

unsigned net_packet_buffer_peek(net_packet_buffer_t *npb, unsigned short *pflags)
{
	ASSERT(npb != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned usage = _usage(npb);
	if (usage > sizeof(net_packet_buffer_header_t))
	{
		unsigned offset = npb->OutputIndex;
		net_packet_buffer_header_t *hdr = (net_packet_buffer_header_t *)(npb->Buffer + offset);

		unsigned length = NTOH16(hdr->Length);
		ASSERT(length != 0, KERNEL_ERROR_KERNEL_PANIC);
		return length;
	}
	return  0;
}

unsigned net_packet_buffer_pop(net_packet_buffer_t *npb, void *buffer, unsigned buffer_length)
{
	ASSERT(npb != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(buffer != NULL, KERNEL_ERROR_NULL_POINTER);

	unsigned usage = _usage(npb);
	if (usage >= sizeof(net_packet_buffer_header_t))
	{
		unsigned offset = npb->OutputIndex;
		net_packet_buffer_header_t *hdr = (net_packet_buffer_header_t *)(npb->Buffer + offset);
		offset += sizeof(net_packet_buffer_header_t);
		if (offset == npb->Size) offset = 0;

		unsigned length = NTOH16(hdr->Length);
		if (usage >= (sizeof(net_packet_buffer_header_t) + length))
		{
			unsigned *dst = (unsigned *)buffer;
			ASSERT(buffer_length >= length, KERNEL_ERROR_KERNEL_PANIC);

			unsigned chunk = npb->Size - offset;
			if (chunk > length) chunk = length;
			
			for(unsigned i = 0; i < chunk; i += sizeof(unsigned)) 
			{
				*dst++ = *(unsigned *)(npb->Buffer + offset); 
				offset += sizeof(unsigned);
			}
			if (offset == npb->Size) offset = 0;

			if (chunk < length)
			{
				chunk = length - chunk;

				for(unsigned i = 0; i < chunk; i += sizeof(unsigned)) 
				{
					*dst++ = *(unsigned *)(npb->Buffer + offset); 
					offset += sizeof(unsigned);
				}
				if (offset == npb->Size) offset = 0;
			}

			npb->OutputIndex = offset;
			return length;
		}
	}
	return 0;
}









