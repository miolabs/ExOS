#ifndef NET_NET_H
#define NET_NET_H

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[6];
} HW_ADDR;

typedef union __attribute__((__packed__))
{
	unsigned char Bytes[4];
	unsigned long Value;
} NET32_T;

typedef struct __attribute__((__packed__))
{
	unsigned char MSB;
	unsigned char LSB;
} NET16_T;

#define NTOH16(s) (((s).LSB)|((s).MSB<<8))
#define HTON16(v) (NET16_T){(unsigned char)((v)>>8), (unsigned char)(v)} 
#define NTOH32(s) (((s).Bytes[3])|((s).Bytes[2]<<8)|((s).Bytes[1]<<16)|((s).Bytes[0]<<24))
#define HTON32(v) (NET32_T){(unsigned char)((v)>>24), (unsigned char)((v)>>16), (unsigned char)((v)>>8), (unsigned char)(v)} 

typedef union __attribute__((__packed__))
{
	unsigned char Bytes[4];
	unsigned long Value;
} IP_ADDR;

// prototypes
void net_initialize();
int net_equal_hw_addr(HW_ADDR *a, HW_ADDR *b);

#endif // NET_NET_H
