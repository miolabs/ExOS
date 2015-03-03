#include "ppp.h"
#include <net/net.h>
#include <support/services/debug.h>
#include <support/misc/crc16.h>
#include "lcp.h"
#include <kernel/machine/hal.h>

static unsigned short _crc_table[256];

static PPP_PARSE_ERROR _parse_frame(PPP_CONTEXT *context, int length)
{
	int offset = 0;
	unsigned char address = 0xff;
	unsigned char control; 
	PPP_PROTOCOL protocol;
	
	if (!(context->Config & PPP_CONFIGF_ADDRESS_OMIT))
	{
		address = context->Frame[offset++];
		if (address != 0xFF) return PPP_PARSE_OK;	// FIXME
	}
	if (!(context->Config & PPP_CONFIGF_CONTROL_OMIT))
	{
		control = context->Frame[offset++];
	}

	if (context->Config & PPP_CONFIGF_SHORT_PROTOCOL)
	{
		protocol = context->Frame[offset++];	// FIXME
	}
	else
	{
		protocol = NTOH16(*(NET16_T *)(context->Frame + offset));
		offset += 2;
	}

	int payload_length = length - (offset + 2);
	
	unsigned short fcs = NTOH16(*(NET16_T *)(context->Frame + offset + payload_length));
	unsigned short crc = crc16_do(context->Frame, payload_length + offset, _crc_table);
	if (crc != fcs)
		return PPP_PARSE_BAD_CRC;
	
	if (payload_length > 0)
	{
		switch(protocol)
		{
			case PPP_PROTOCOL_LCP:
				ppp_lcp_parse(context, context->Frame + offset, payload_length);
		}
	}
}

#define PPP_BUFFER_SIZE 500

static int _open(COMM_IO_ENTRY *io, const char *path, int baudrate)
{
	EXOS_TREE_DEVICE *dev_node = (EXOS_TREE_DEVICE *)exos_tree_find_path(NULL, path);
	if (dev_node != NULL && dev_node->Type == EXOS_TREE_NODE_DEVICE)
	{
		comm_io_create(io, dev_node->Device, dev_node->Unit, EXOS_IOF_NONE);
		comm_io_set_attr(io, COMM_ATTR_BAUDRATE, (void *)&baudrate);
		int res = comm_io_open(io);
		return res == 0;
	}
	return 0;
}	

void ppp_loop(PPP_CONTEXT *context, const char *path, int baudrate)
{
	COMM_IO_ENTRY io;
	if (!_open(&io, path, baudrate))
		return;
	context->Link = (EXOS_IO_ENTRY *)&io;
	crc16_initialize(_crc_table, 0x1021);
	
	unsigned short buffer[PPP_BUFFER_SIZE];	// NOTE: quite a lot of stack!
	int offset = 0;
	int running = 1;
	int timeout = 1;
	int rem = 0;
	while(running)
	{
		int done = exos_io_read(context->Link, buffer + rem, PPP_BUFFER_SIZE - rem);
		if (done < 0) break;

		rem = 0;
		for (int i = 0; i < done; i++)
		{
			unsigned char c = buffer[i++];
			if (c == 0x7e)
			{
				PPP_PARSE_ERROR error = _parse_frame(context, offset);
#ifdef PPP_VERBOSE
				if (error != PPP_PARSE_OK)
					debug_printf("PPP: frame parse error %d.\r\n", error);
#endif
				offset = 0;
			}
			else if (offset < context->MRU)
			{
				// de-stuffing
				if (c == 0x7d)
				{
					if (i < done - 1)
					{
						c = buffer[i++];
						context->Frame[offset++] = c ^ 0x20;
					}
					else buffer[rem++] = c;
				}
				else context->Frame[offset++] = c;
			}
		}

		if (done == 0)
		{
			if (timeout < 500)
				timeout *= 2;
			if (running)
				exos_event_wait(&context->Link->InputEvent, timeout);
		}
		else timeout = 1;
	}

#ifdef PPP_VERBOSE
	debug_printf("PPP: I/O Error, exitting!\r\n");
#endif
}
