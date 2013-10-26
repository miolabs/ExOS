#ifndef XCPU_PERSIST_H
#define XCPU_PERSIST_H

#define XCPU_PERSIST_MAGIC (('X') | ('C' << 8) | ('P' << 16) | (2 << 24))

typedef struct
{
	unsigned long Magic;
	unsigned long TotalSteps;
	unsigned char ConfigBits;
	unsigned char DriveMode;
	unsigned char Reserved;
	signed char WheelRatioAdj;
	unsigned char ThrottleAdjMin, ThrottleAdjMax;
	unsigned char CustomCurve[7];
} XCPU_PERSIST_DATA;

typedef enum
{
	XCPU_CONFIGF_NONE = 0,
	XCPU_CONFIGF_MILES = (1<<0),
} XCPU_CONFIG_FLAGS;

// prototypes
int persist_load(XCPU_PERSIST_DATA *data);
int persist_save(const XCPU_PERSIST_DATA *data);
void persist_enter_bootloader();


#endif // XCPU_PERSIST_H

