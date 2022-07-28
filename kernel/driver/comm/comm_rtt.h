#ifndef RTT_DEVICE_H
#define RTT_DEVICE_H

typedef struct
{
	const char *Name;
	char *BufStart;
	unsigned BufSize;
	volatile unsigned WriteOffset;
	volatile unsigned ReadOffset;
	unsigned Flags;
} rtt_buffer_t;

typedef struct
{
	const char Id[16];
	unsigned MaxUpBuffers, MaxDownBuffers;
} rtt_control_block_t;

void rtt_buffer_create(rtt_buffer_t *rtt, const char *name, void *buf, unsigned size);
int rtt_read(rtt_buffer_t *rtt, void *buf, unsigned int length);
int rtt_write(rtt_buffer_t *rtt, const void *buf, unsigned int length);

#endif // RTT_DEVICE_H

