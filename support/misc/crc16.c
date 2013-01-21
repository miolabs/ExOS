// CRC16/CRC7 Generation
// by Miguel Fides
// 
// CRC16 based on algorithms and publications by Ross N. Williams
// See: http://www.geocities.com/SiliconValley/Pines/8659/crc.htm

#include "crc16.h"

/// crc16 table generator
void crc16_initialize(unsigned short *table, unsigned short poly)
{
	int i;
	int b;
	for(b = 0; b < 256; b++)
	{
		unsigned short acc = b << 8;
		for (i = 0; i < 8; i++)
		{
			acc = (acc & 0x8000) ? ((acc << 1) ^ poly) : (acc << 1);
		}
		table[b] = acc;
	}
}

unsigned short crc16_do(unsigned char *data, int length, unsigned short *table)
{
	unsigned short crc = 0;
	for(int i = 0; i < length; i++) CRC16(table, crc, data[i]);
	return crc;
}

/// crc7 tableless (slow, direct, cheap) encoding
/// crc7 is used by selected (rare) commands for SD cards (not for reading blocks)
static unsigned char _encode(unsigned char seed, unsigned char input, unsigned char depth)
{
	register unsigned char regval;      // shift register byte.
	register unsigned char count;
	register unsigned char cc;          // data to manipulate.
	#define POLYNOM (0x9)        // polynomical value to XOR when 1 pops out.
	
	regval = seed;    // get prior round's register value.
	cc = input;       // get input byte to generate CRC, MSB first.
	
	for (count = depth; count--; cc <<= 1)
	{
		regval = (regval << 1) | ((cc & 0x80) ? 1 : 0);
		if (regval & 0x80) regval ^= POLYNOM;
	}
	return (regval & 0x7f);    // return lower 7 bits of CRC as value to use.
}

unsigned char crc7_do(unsigned char *data, int length)
{
	unsigned char crc = 0;
	for(int i = 0; i < length; i++)
	{
		crc = _encode(crc, data[i], 8);
	}
	crc = _encode(crc, 0, 7);
	return (crc << 1) | 1;
}
