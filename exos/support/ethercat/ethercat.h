#ifndef NET_ETHERCAT_H
#define NET_ETHERCAT_H

#include <net/net.h>
#include <net/mbuf.h>
#include <net/adapter.h>

typedef struct __attribute__((__packed__))
{
	unsigned short Header;
} ECAT_HEADER;

typedef enum
{
	ECAT_FRAME_PROTOCOL = 1,
	ECAT_FRAME_PROCESS_DATA = 4,
	ECAT_FRAME_MAILBOX_DATA = 5,
} ECAT_FRAME_TYPE;

typedef struct __attribute__((__packed__))
{
	unsigned char Command;
	unsigned char Index;
	unsigned short SlaveAddress;
	unsigned short OffsetAddress;
	unsigned short Length;
	unsigned short Interrupt;
} ECAT_DATAGRAM_HEADER;

#define ECAT_DGRAM_LENGTH_ROUND_TRIP (1<<14)
#define ECAT_DGRAM_LENGTH_NOT_LAST (1<<15)

typedef struct
{
	EXOS_NODE;
	ECAT_DATAGRAM_HEADER Header;
	NET_MBUF HeaderBuffer;
	NET_MBUF DataBuffer;
	NET_MBUF FooterBuffer;
	unsigned short WorkCounter;
} ECAT_DATAGRAM;

// prototypes
int net_ecat_input(NET_ADAPTER *adapter, ETH_HEADER *buffer, ECAT_HEADER *ecat);

void net_ecat_datagram_create(ECAT_DATAGRAM *dgram, void *data, int offset, int length);
int net_ecat_build_buffer(NET_MBUF *ecat_mbuf, ECAT_HEADER *ecat, ECAT_FRAME_TYPE ecat_type, EXOS_LIST *datagram_list);
int net_ecat_send(NET_ADAPTER *adapter, EXOS_LIST *datagram_list);

#endif // NET_ETHERCAT_H


