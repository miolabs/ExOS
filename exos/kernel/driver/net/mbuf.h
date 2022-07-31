#ifndef NET_MBUF_H
#define NET_MBUF_H

#include <kernel/types.h>

typedef struct _NET_MBUF
{
	struct _NET_MBUF *Next;
	void *Buffer;
	int Offset;
	int Length;
} NET_MBUF;

// prototypes
void net_mbuf_init(NET_MBUF *buf, void *data, int offset, int length);
int net_mbuf_append(NET_MBUF *buf, NET_MBUF *append);
int net_mbuf_length(NET_MBUF *mbuf);
int net_mbuf_seek(NET_MBUF *mbuf_seek, NET_MBUF *mbuf_base, int seek);

int net_mbuf_copy_contents(NET_MBUF *target, NET_MBUF *source);
void net_mbuf_copy_mem(void *src, void *dst, unsigned size);

#endif // NET_MBUF_H
