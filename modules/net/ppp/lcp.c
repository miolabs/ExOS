#include "lcp.h"
#include <net/net.h>

static unsigned long _magic_number = 0;

static void _parse_options(PPP_CONTEXT *context, unsigned char *buffer, int length);

void ppp_lcp_parse(PPP_CONTEXT *context, unsigned char *buffer, int length)
{
	if (length >= 4)
	{
		int offset = 0;
		LCP_CODE code = (LCP_CODE)buffer[offset++];
		unsigned char identifier = buffer[offset++];
		unsigned short lcp_length = NTOH16(*(NET16_T *)(buffer + offset));
		offset += 2;

		if (lcp_length <= length)
		{
			switch(code)
			{
				case LCP_CONFIGURE_REQUEST:
					_parse_options(context, buffer + offset, lcp_length - offset);
					break;
				case LCP_CONFIGURE_ACK:
				case LCP_CONFIGURE_NAK:
				case LCP_CONFIGURE_REJECT:
				case LCP_TERMINATE_REQUEST:
				case LCP_TERMINATE_ACK:
					//TODO
					break;
			}
		}
	}
}

static void _parse_options(PPP_CONTEXT *context, unsigned char *buffer, int length)
{
	int offset = 0;
	do 
	{
		LCP_OPTION_TYPE type = (LCP_OPTION_TYPE)buffer[offset++];
		int op_length = buffer[offset++];
		switch(type)
		{
			case LCP_OPTION_MRU:
			case LCP_OPTION_AUTHENTICATION_PROTOCOL:
			case LCP_OPTION_QUALITY_PROTOCOL:
			case LCP_OPTION_MAGIC_NUMBER:
			case LCP_OPTION_PROTOCOL_FIELD_COMPRESSION:
			case LCP_OPTION_ADDRESS_AND_CONTROL_FIELD_COMPRESSION:
				// TODO
				break;
		}
	} while(offset < length);
}
