#ifndef NET_NET_H
#define NET_NET_H

typedef struct __attribute__((__packed__))
{
	unsigned char Bytes[6];
} hw_addr_t;

typedef union __attribute__((__packed__))
{
	unsigned char Bytes[4];
	unsigned long Value;
} net32_t;

typedef struct __attribute__((__packed__))
{
	unsigned char MSB;
	unsigned char LSB;
} net16_t;

#define NTOH16(s) (((s).LSB)|((s).MSB<<8))
#define HTON16(v) (net16_t){(unsigned char)((v)>>8), (unsigned char)(v)} 
#define NTOH32(s) (((s).Bytes[3])|((s).Bytes[2]<<8)|((s).Bytes[1]<<16)|((s).Bytes[0]<<24))
#define HTON32(v) (net32_t){(unsigned char)((v)>>24), (unsigned char)((v)>>16), (unsigned char)((v)>>8), (unsigned char)(v)} 

#ifdef EXOS_OLD
typedef hw_addr_t HW_ADDR;
typedef net32_t NET32_T;
typedef net16_t NET16_T;
#endif

// prototypes
void net_initialize();
int net_equal_hw_addr(hw_addr_t *a, hw_addr_t *b);

#endif // NET_NET_H
