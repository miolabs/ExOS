#ifndef NORDIC_ACI_H
#define NORDIC_ACI_H

#include <kernel/event.h>

typedef struct
{
	EXOS_NODE Node;
	unsigned char Command;
	unsigned char Length;
	unsigned char Data[32];
} ACI_COMMAND;

void aci_initialize();

#endif // NORDIC_ACI_H


