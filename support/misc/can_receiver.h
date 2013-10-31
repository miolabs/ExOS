#ifndef MISC_CAN_RECEIVER_H
#define MISC_CAN_RECEIVER_H

#include <support/can_hal.h>
#include <kernel/port.h>

typedef struct
{
	EXOS_MESSAGE;
	CAN_MSG CanMsg;
} CAN_RX_MSG;

typedef struct
{
	EXOS_NODE Node;
	EXOS_PORT RxPort;
	unsigned long Bus;
	unsigned long Id;
	unsigned long IdMask;
} CAN_HANDLER;

void can_receiver_initialize();
int can_receiver_add_handler(CAN_HANDLER *handler, int bus, unsigned long id, unsigned long id_mask);
int can_receiver_read(CAN_HANDLER *handler, CAN_MSG *msg, int timeout);


#endif // MISC_CAN_RECEIVER_H
