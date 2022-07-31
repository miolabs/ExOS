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
	ETH_TYPE_ETHERCAT = 0x88a4,
} ETH_TYPE;

#define ETH_MAX_FRAME_SIZE 1536
#define ETH_MAX_PAYLOAD 1500

typedef struct _NET_DRIVER NET_DRIVER;

typedef struct
{
	node_t Node;
	const NET_DRIVER *Driver;
	mutex_t InputLock;
	mutex_t OutputLock;
	HW_ADDR MAC;
	unsigned short Speed;
	IP_ADDR IP;
	IP_ADDR NetMask;
	IP_ADDR Gateway;

	unsigned long InputSignal;
	exos_thread_t Thread;
	unsigned char Stack[NET_ADAPTER_THREAD_STACK];
} NET_ADAPTER;

typedef struct
{
	node_t Node;
	NET_ADAPTER *Adapter;
	void *Buffer;
	unsigned short Offset;
	unsigned short Length;
} NET_BUFFER;

typedef struct
{
	event_t *CompletedEvent;
	NET_MBUF Buffer;
} NET_OUTPUT_BUFFER;

typedef void(* NET_CALLBACK)(void *state);

struct _NET_DRIVER
{
	int (*Initialize)(NET_ADAPTER *adapter);
	void (*LinkUp)(NET_ADAPTER *adapter);
	void (*LinkDown)(NET_ADAPTER *adapter);
	void *(*GetInputBuffer)(NET_ADAPTER *adapter, unsigned long *plength);
	void (*DiscardInputBuffer)(NET_ADAPTER *adapter, void *buffer);
	void *(*GetOutputBuffer)(NET_ADAPTER *adapter, unsigned long size);
	int (*SendOutputBuffer)(NET_ADAPTER *adapter, NET_MBUF *mbuf, NET_CALLBACK callback, void *state);
};

// prototypes
void net_adapter_initialize();
int net_adapter_install(NET_ADAPTER *adapter);
void net_adapter_list_lock();
void net_adapter_list_unlock();
int net_adapter_enum(NET_ADAPTER **padapter);
NET_ADAPTER *net_adapter_find(IP_ADDR addr);
NET_ADAPTER *net_adapter_find_gateway(IP_ADDR addr);

void net_adapter_input(NET_ADAPTER *adapter);

void *net_adapter_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *buf, unsigned hdr_size, const HW_ADDR *destination, ETH_TYPE type);
int net_adapter_send_output(NET_ADAPTER *adapter, NET_OUTPUT_BUFFER *buf);

NET_BUFFER *net_adapter_alloc_buffer(NET_ADAPTER *adapter, void *buffer, void *data, unsigned long length);
void net_adapter_discard_input_buffer(NET_BUFFER *packet);


#endif // NET_DRIVERS_H

