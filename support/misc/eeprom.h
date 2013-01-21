#ifndef MISC_EEPROM_H
#define MISC_EEPROM_H

typedef enum
{
	EEPROM_RES_OK = 0,
	EEPROM_RES_NO_RESPONSE = 3,
} EEPROM_RESULT;

// prototypes
int eeprom_initialize();
int eeprom_read_geometry(int *psize, int *ppagesize);
EEPROM_RESULT eeprom_read(unsigned char *buf, int offset, int length);
EEPROM_RESULT eeprom_write(unsigned char *buf, int offset, int length);

#endif