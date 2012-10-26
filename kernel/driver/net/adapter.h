#ifndef NET_ADAPTER_H
#define NET_ADAPTER_H

#include "net.h"
#include "mbuf.h"
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/mutex.h>

#ifndef NET_ADAPTER_THREAD_STACK
#define NET_ADAPTER_THREAD_STACK 768
#endif

typedef struct __attribute__((__packed__))
{
	HW_ADDR Destination;
	HW_ADDR Sender;
	NET16_T Type;
} ETH_HEADER;

typedef enum
{
	ETH_TYPE_ARP = 0x0806,
	ETH_TYPE_IP = 0x0800,
	ETH_TYPE_IPv6 = 0x86dd,
} ETH_TYPE;

#define ETH_MAX_FRAME_SIZE 1536
#define ETH_MAX_PAYLOAD 1500

typedef struct _ETH_DRIVER ETH_DRIVER;

typedef struct
{
	EXOS_NODE Node;
	const ETH_DRIVER *Driver;
	EXOS_MUTEX InputLock;
	EXOS_MUTEX OutputLock;
	HW_ADDR MAC;
	unsigned short Speed;
	IP_ADDR IP;
	IP_ADDR NetMask;
	IP_ADDR Gateway;

	unsigned long InputSignal;
	EXOS_THREAD Thread;
	unsigned char Stack[NET_ADAPTER_THREAD_STACK];
} ETH_ADAPTER;

typedef struct
{
	EXOS_NODE Node;
	ETH_ADAPTER *Adapter;
	void *Buffer;
	unsigned short Offset;
	unsigned short Length;
} ETH_BUFFER;

typedef struct
{
	EXOS_EVENT *CompletedEvent;
	NET_MBUF Buffer;
} ETH_OUTPUT_BUFFER;

struct _ETH_DRIVER
{
	int (*Initialize)(ETH_ADAPTER *adapter);
	void (*LinkUp)(ETH_ADAPTER *adapter);
	void (*LinkDown)(ETH_ADAPTER *adapter);
	ETH_HEADER *(*GetInputBuffer)(ETH_ADAPTER *adapter, unsigned long *plength);
	void (*DiscardInputBuffer)(ETH_ADAPTER *adapter, ETH_HEADER *buffer);
	ETH_HEADER *(*GetOutputBuffer)(ETH_ADAPTER *adapter, unsigned long size);
	int (*SendOutputBuffer)(ETH_ADAPTER *adapter, NET_MBUF *mbuf, ETH_CALLBACK callback, void *state);
};

// prototypes
void net_adapter_initialize();
void net_adapter_list_lock();
void net_adapter_list_unlock();
int net_adapter_enum(ETH_ADAPTER **padapter);
ETH_ADAPTER *net_adapter_find(IP_ADDR addr);

void net_adapter_input(ETH_ADAPTER *adapter);

void *net_adapter_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *buf, unsigned hdr_size, HW_ADDR *destination, ETH_TYPE type);
int net_adapter_send_output(ETH_ADAPTER *adapter, ETH_OUTPUT_BUFFER *buf);

ETH_BUFFER *net_adapter_alloc_buffer(ETH_ADAPTER *adapter, void *buffer, void *data, unsigned long length);
void net_adapter_discard_input_buffer(ETH_BUFFER *packet);


#endif // NET_DRIVERS_H

