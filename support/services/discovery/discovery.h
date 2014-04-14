#ifndef EXOS_DISCOVERY_H
#define EXOS_DISCOVERY_H

typedef enum
{
	DISCOVERY_CMD_DISCOVER = 0,
	DISCOVERY_CMD_RESET,
	DISCOVERY_CMD_RECONFIG,
	DISCOVERY_CMD_READY,
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

void discovery_loop();

#endif // EXOS_DISCOVERY_H

