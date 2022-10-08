#ifndef NET_MBUF_H
#define NET_MBUF_H

#include <kernel/types.h>

typedef struct __net_mbuf
{
	struct __net_mbuf *Next;
	void *Buffer;
	int Offset;
	int Length;
} net_mbuf_t;

#ifdef EXOS_OLD
typedef NET_MBUF net_mbuf_t;
#endif

// prototypes
void net_mbuf_init(net_mbuf_t *buf, void *data, int offset, int length);
int net_mbuf_append(net_mbuf_t *buf, net_mbuf_t *append);
int net_mbuf_length(net_mbuf_t *mbuf);
int net_mbuf_seek(net_mbuf_t *mbuf_seek, net_mbuf_t *mbuf_base, int seek);

int net_mbuf_copy_contents(net_mbuf_t *target, net_mbuf_t *source);
void net_mbuf_copy_mem(void *src, void *dst, unsigned size);

#endif // NET_MBUF_H
