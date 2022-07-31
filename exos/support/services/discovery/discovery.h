#ifndef EXOS_DISCOVERY_H
#define EXOS_DISCOVERY_H

typedef enum
{
	DISCOVERY_CMD_DISCOVER = 0,
	DISCOVERY_CMD_QUIT,
	DISCOVERY_CMD_GET_CONFIG,
	DISCOVERY_CMD_SET_CONFIG,
	DISCOVERY_CMD_READY = 16,
} DISCOVERY_CMD;

typedef struct
{
	unsigned long Magic;
	unsigned long Version;
	DISCOVERY_CMD Command;
	unsigned char Data[0];
} DISCOVERY_MSG;

typedef struct
{
	unsigned long IP;
	unsigned long Mask;
	unsigned long Services;
} DISCOVERY_CONFIG;

#ifndef DISCOVERY_PORT
#define DISCOVERY_PORT 18180
#endif

typedef struct
{
	int (*FillConfig)(void *data);
	void (*ConfigReceived)(void *data, int length);
} DISCOVERY_CONFIG_CALLBACK;

void discovery_loop(const DISCOVERY_CONFIG_CALLBACK *callback);

#endif // EXOS_DISCOVERY_H

