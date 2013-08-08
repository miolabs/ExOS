#ifndef XKUTY_IAP_H
#define XKUTY_IAP_H

#include "xdisplay.h"

#define XIAP_MAGIC ('X' | ('K' << 8) | ('U' << 16) | ('1' << 24)) 

typedef struct __attribute__((__packed__))
{
	unsigned long Magic;
	unsigned short Speed;
	unsigned short StatusFlags;
	unsigned long Distance;
	unsigned char Battery;
} XIAP_FRAME;


void xiap_send_frame(DASH_DATA *dash);

#endif // XKUTY_IAP_H