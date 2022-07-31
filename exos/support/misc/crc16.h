#ifndef CRC16_H
#define CRC16_H

// NOTE: remainder must be of type _unsigned_ short
#define CRC16(table, rem, data) (rem) = (((rem) << 8) ^ table[((rem) >> 8) ^ (data)])

void crc16_initialize(unsigned short *table, unsigned short poly);
unsigned short crc16_do(unsigned char *data, int length, unsigned short *table);
unsigned char crc7_do(unsigned char *data, int length);

#endif //CRC16_H
