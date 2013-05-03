#ifndef XCPU_PERSIST_H
#define XCPU_PERSIST_H

#define XCPU_PERSIST_MAGIC (('X') | ('C' << 8) | ('P' << 16) | (1 << 24))

typedef struct
{
	unsigned long Magic;
	unsigned long TotalSteps;
	unsigned char ConfigBits;
	signed char WheelRatioAdj;
	unsigned short Reserved;
} XCPU_PERSIST_DATA;

typedef enum
{
	XCPU_CONFIGF_NONE = 0,
	XCPU_CONFIGF_MILES = (1<<0),
} XCPU_CONFIG_FLAGS;

// prototypes
int persist_load(XCPU_PERSIST_DATA *data);
int persist_save(const XCPU_PERSIST_DATA *data);



#endif // XCPU_PERSIST_H

