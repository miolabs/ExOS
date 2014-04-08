#ifndef XCPU_PERSIST_H
#define XCPU_PERSIST_H

#include "phone_manager.h"

#define XCPU_PERSIST_MAGIC (('X') | ('C' << 8) | ('P' << 16) | (3 << 24))

#define XCPU_PERSIST_MAX_SIZE 248

// Limit this structure to 248 bytes max., limit of the EEPROM
typedef struct
{
	unsigned long Magic;
	unsigned long TotalSteps;
	unsigned char ConfigBits;
	unsigned char DriveMode;
	signed char   WheelRatioAdj;
	unsigned char Reserved1;
   	// aligned 4
	unsigned char ThrottleAdjMin, ThrottleAdjMax;
	unsigned char MaxSpeed;
	unsigned char CustomCurve[7];
	unsigned short Reserved2;  
	// aligned 4
	XCPU_PHONE_REG Phones[XCPU_PHONE_LOGS];	// Phones allowed to connect
	// aligned 4
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

