#ifndef APPLE_IAP_COMM_H
#define APPLE_IAP_COMM_H

#include <kernel/iobuffer.h>
#include <kernel/io.h>

#ifndef APPLE_IAP_IO_BUFFER
#define APPLE_IAP_IO_BUFFER 256
#endif

typedef enum
{
	APPLE_IAP_IO_UNAVAILABLE = 0,
	APPLE_IAP_IO_CLOSED,
	APPLE_IAP_IO_OPENED,
	APPLE_IAP_IO_DETACHED,
} APPLE_IAP_IO_STATE;

typedef struct
{
	node_t Node;
	const char *Name;
	io_tree_device_t KernelDevice;

	unsigned short ProtocolIndex;
	unsigned short SessionID;

	io_entry_t *Entry;
	APPLE_IAP_IO_STATE IOState;

#ifdef DEBUG
	unsigned int write_req_cnt;
	unsigned int write_byte_cnt;
#endif

	event_t InputEvent;
	io_buffer_t InputIOBuffer;
   	unsigned char InputBuffer[APPLE_IAP_IO_BUFFER];	
	unsigned char OutputBuffer[APPLE_IAP_IO_BUFFER];
} APPLE_IAP_PROTOCOL_MANAGER;

void iap_comm_initialize();
void iap_comm_add_protocol(APPLE_IAP_PROTOCOL_MANAGER *iap);
APPLE_IAP_PROTOCOL_MANAGER *iap_comm_get_protocol(int index);
int iap_comm_get_protocol_count();
int iap_open_session(unsigned short session_id, unsigned short protocol);
void iap_close_session(unsigned short session_id);
void iap_close_all();
int iap_comm_write(unsigned short session_id, unsigned char *buffer, int length);

#endif // APPLE_IAP_COMM_H



