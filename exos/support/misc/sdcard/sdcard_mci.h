#ifndef SDCARD_MCI_H
#define SDCARD_MCI_H

#include "sdcard.h"

typedef enum _MCI_WAIT_FLAGS
{
	MCI_WAIT_RESPONSE = 1,
	MCI_WAIT_LONGRESP = 2,
	MCI_WAIT_CRC = 4,
	MCI_WAIT_CMD = 8,
} MCI_WAIT_FLAGS;

typedef enum
{
	MCI_BLOCK_512 = 9,
} MCI_BLOCK_SIZE; 


#endif // SDCARD_MCI_H

