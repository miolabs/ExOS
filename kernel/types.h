#ifndef EXOS_TYPES_H
#define EXOS_TYPES_H

#ifndef __weak
#define __weak __attribute__((__weak__))
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define __ABS(V)     ((V<0) ? -(V) : (V))  
#define __MIN(A,B)   ((A<B) ? (A) : (B))  
#define __MAX(A,B)   ((A>B) ? (A) : (B)))  
#define __LIMIT(A,MIN,MAX)  (((A)<(MIN)) ? (MIN) : (((A)>(MAX)) ? (MAX) : (A)))
#define __SWAP(TYPE,A,B)    { TYPE tmp = (B); B = (A); A = tmp; }

#define MAXINT (0x7FFFFFFFL)

#ifndef __bool_is_defined
#define __bool_is_defined 
typedef int bool;
#endif

#define false 0
#define true 1

typedef union
{
	unsigned long Longs[4];
	unsigned char Bytes[16];
} GUID;

extern const GUID GUID_NULL;

static inline bool __guid_eq(GUID *guid1, GUID *guid2)
{
	return guid1->Longs[0] == guid2->Longs[0] && guid1->Longs[1] == guid2->Longs[1] &&
		guid1->Longs[2] == guid2->Longs[2] && guid1->Longs[3] == guid2->Longs[3];
}

#endif // EXOS_TYPES_H
