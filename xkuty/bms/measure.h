#ifndef BMS_MEASURE_H
#define BMS_MEASURE_H

#ifndef CELL_COUNT
#define CELL_COUNT 15
#endif

typedef enum
{
	MEASURE_OK = 0,
	MEASURE_SUPERVISOR_FAILURE,
	MEASURE_VIN_MISMATCH,
} MEASURE_ERROR;

// prototypes
int measure_initialize();
MEASURE_ERROR measure_update();
int measure_vbat();
int measure_current();
int measure_soc();
int measure_vchg();
int measure_cell_balance(int *pcell);

#endif // BMS_MEASURE_H

