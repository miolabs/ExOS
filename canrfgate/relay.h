#ifndef RELAY_H
#define RELAY_H

#include <kernel/port.h>

typedef struct
{
	EXOS_MESSAGE;
	unsigned short Time;
	unsigned short Unit;
	unsigned long RelayMask;
} RELAY_MSG;

void relay_initialize();
void relay_set(int unit, unsigned long mask, unsigned long time);

int open_relay(int unit, unsigned long mask, unsigned long time);

#endif // RELAY_H


