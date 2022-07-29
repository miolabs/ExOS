#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

typedef struct
{
	unsigned long IP;
	unsigned long NetMask;
	unsigned long Services;
} NET_CONFIG;

typedef struct
{
	NET_CONFIG Network;
} SERVER_CONFIG;

extern SERVER_CONFIG __config;


#endif // SERVER_CONFIG_H

