#ifndef XCPU_CAN_COMMS_H
#define XCPU_CAN_COMMS_H

#include <xkuty/xcpu.h>



void xcpu_can_initialize();
void xcpu_can_send_messages(XCPU_MASTER_OUT1 *report, XCPU_MASTER_OUT2 *adj);
XCPU_MSG *xcpu_can_get_message();
void xcpu_can_release_message(XCPU_MSG *xmsg);

void xcpu_received_master_input(XCPU_MASTER_INPUT *);


#endif // XCPU_CAN_COMMS_H
