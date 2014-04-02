#ifndef SERVER_H
#define SERVER_H

void server_start();

// callbacks
int set_relay(int unit, unsigned short mask, unsigned short value, unsigned long time);

#endif // SERVER_H
