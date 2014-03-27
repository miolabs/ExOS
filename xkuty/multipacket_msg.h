#ifndef MULTIPACKET_MSG__H
#define MULTIPACKET_MSG__H

#include <support/can_hal.h>

#define MULTIPACKET_MAX_LEN   128

// Loads (copy) a multipacket msg to be sent. Max len is MULTIPACKET_MAX_LEN bytes
// If a msg is being sent, returns 0
// If the module is ready to send a new message, returns a buffer pointer
unsigned char* multipacket_msg_reset ( int len);

// This module send 2 CAN packets each iteration to improve speed
// If a prev. msg is still pending, returns 0
int multipacket_msg_send ( int id1); //, int id2);

// Appends read info until buffer is complete. If complete, return pointer & len
// If msg was received uncomplete, returns len -1
const unsigned char* multipacket_msg_receive (int* recv_len, const CAN_MSG* msg);

#endif // MULTIPACKET_MSG__H
