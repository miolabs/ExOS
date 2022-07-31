#ifndef EXOS_NET_PPP_H
#define EXOS_NET_PPP_H

#include <comm/comm.h>

typedef enum
{
	PPP_PROTOCOL_LCP = 0xc021,	// Link Control Protocol
	PPP_PROTOCOL_PAP = 0xc023,	// Password Authentication Protocol
	PPP_PROTOCOL_LQR = 0xc025,	// Link Quality Report
	PPP_PROTOCOL_CHAP = 0xc223,	// Challenge Handshake Authentication Protocol
} PPP_PROTOCOL;

typedef enum
{
	PPP_STA_DEAD = 0,
	PPP_STA_ESTABLISH,
	PPP_STA_AUTHENTICATE,
	PPP_STA_NETWORK,
	PPP_STA_TERMINATE,
} PPP_STATE;

typedef enum
{
	PPP_OPT_INITIAL = 0,
	PPP_OPT_STARTING, PPP_OPT_CLOSED,
	PPP_OPT_STOPPED, PPP_OPT_CLOSING, PPP_OPT_STOPPING,
	PPP_OPT_REQ_SENT, PPP_OPT_ACK_RCVD, PPP_OPT_ACK_SENT, 
	PPP_OPT_OPENED,
} PPP_OPT_STATE;

// implementation structures

typedef enum
{
	PPP_CONFIGF_ADDRESS_OMIT = (1<<8),
	PPP_CONFIGF_CONTROL_OMIT = (1<<9),
	PPP_CONFIGF_SHORT_PROTOCOL = (1<<10),
	PPP_CONFIGF_LONG_FCS = (1<<11),

} PPP_CONFIG_FLAGS;

#define PPP_MAX_MRU 1500

typedef struct
{
	PPP_CONFIG_FLAGS Config;
	EXOS_IO_ENTRY *Link;
	unsigned short MRU;
	unsigned char Frame[PPP_MAX_MRU];
} PPP_CONTEXT;

typedef enum
{
	PPP_PARSE_OK = 0,
	PPP_PARSE_IO_ERROR = 1,
	PPP_PARSE_BAD_CRC = 2,
} PPP_PARSE_ERROR;

// prototypes
void ppp_loop(PPP_CONTEXT *context, const char *path, int baudrate);

#endif // EXOS_NET_PPP_H
