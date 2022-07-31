// IP Stack. MBuf Management
// by Miguel Fides

#include "mbuf.h"
#include "net.h"

void net_mbuf_init(NET_MBUF *buf, void *data, int offset, int length)
{
	buf->Next = NULL;
	buf->Buffer = data;
	buf->Offset = offset;
	buf->Length = length;
}

int net_mbuf_append(NET_MBUF *buf, NET_MBUF *append)
{
	while(1)
	{
		if (buf == append) return 0;
		if (buf->Next == NULL) break;
		buf = buf->Next;
	}
	buf->Next = append;
	return 1;
}

int net_mbuf_length(NET_MBUF *mbuf)
{
	int length = 0;
	while(mbuf != NULL)
	{
		length += mbuf->Length;
		mbuf = mbuf->Next;
	}
	return length;
}

int net_mbuf_seek(NET_MBUF *mbuf_seek, NET_MBUF *mbuf_base, int seek)
{
	int done = 0;
	NET_MBUF *mbuf = mbuf_base;
	while (mbuf != NULL)
	{
		if ((mbuf->Offset + seek) < mbuf->Length)
		{
			*mbuf_seek = *mbuf;
			mbuf_seek->Offset += seek;
			done = 1;
			break;
		}
		seek -= (mbuf->Length - mbuf->Offset);
		mbuf = mbuf->Next;
	}
	return done;
}

int net_mbuf_copy_contents(NET_MBUF *target, NET_MBUF *source)
{
	int target_len, source_len;
	unsigned char *target_ptr;
	unsigned char *source_ptr;
	int copied = 0;

	if (target != NULL)
	{
		target_len = target->Length;
		target_ptr = target->Buffer + target->Offset;
	}
	else return 0;
	if (source != NULL)
	{
		source_len = source->Length;
		source_ptr = source->Buffer + source->Offset;
	}
	else return 0;

	while(1)
	{
		int length = (target_len < source_len) ? target_len : source_len;
		net_mbuf_copy_mem(source_ptr, target_ptr, length); 
		source_ptr += length;
		target_ptr += length;

		//for (int i = 0; i < length; i++) *target_ptr++ = *source_ptr++;
		target_len -= length;
		source_len -= length;
		copied += length;

		if (target_len <= 0)
		{
   			target = target->Next;
			if (target == NULL) break;
			target_len = target->Length;
			target_ptr = target->Buffer + target->Offset;
		}
		if (source_len <= 0)
		{
   			source = source->Next;
			if (source == NULL) break;
			source_len = source->Length;
			source_ptr = source->Buffer + source->Offset;
		}
	}
	return copied;
}

#pragma GCC optimize(2)

__attribute__((__weak__))
void net_mbuf_copy_mem(void *src, void *dst, unsigned size)
{
	while(size >= 2)
	{
		*(unsigned short *)dst = *(unsigned short *)src;
		dst += 2;
		src += 2;
		size -= 2;
	}
	if (size != 0)
		*(unsigned char *)dst = *(unsigned char *)src;
}



