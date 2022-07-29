#ifndef RELAY_H
#define RELAY_H

#include <kernel/port.h>

typedef struct
{
	EXOS_MESSAGE;
	unsigned short Time;
	unsigned short Unit;
	unsigned short Mask;
	unsigned short Value;
} RELAY_MSG;

void relay_initialize();
void relay_set(int unit, unsigned short mask, unsigned short value, unsigned long time);

#endif // RELAY_H


