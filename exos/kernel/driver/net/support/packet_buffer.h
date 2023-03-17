#ifndef NET_PACKET_BUFFER_H
#define NET_PACKET_BUFFER_H

#include <net/net.h>
#include <net/mbuf.h>
#include <kernel/event.h>

typedef struct
{
	event_t Notify;
	unsigned char *Buffer;
	unsigned Size;
	unsigned InputIndex;
	unsigned OutputIndex;
} net_packet_buffer_t;

typedef struct __packed
{
	net16_t Length;
	net16_t Flags; 
} net_packet_buffer_header_t;

void net_packet_buffer_initialize(net_packet_buffer_t *npb, void *data, unsigned size);
unsigned net_packet_buffer_free(net_packet_buffer_t *npb);
bool net_packet_buffer_push(net_packet_buffer_t *npb, void *data, unsigned short length, unsigned short flags);
unsigned net_packet_buffer_peek(net_packet_buffer_t *npb, unsigned short *pflags);
unsigned net_packet_buffer_pop(net_packet_buffer_t *npb, void *buffer, unsigned buffer_length);


#endif //  NET_PACKET_BUFFER_H


