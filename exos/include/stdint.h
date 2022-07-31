#ifndef __stdint_h
#define __stdint_h

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int_fast8_t;
typedef short int_fast16_t;
typedef long int_fast32_t;
typedef long long int_fast64_t;

typedef unsigned char uint_fast8_t;
typedef unsigned short uint_fast16_t;
typedef unsigned long uint_fast32_t;
typedef unsigned long long uint_fast64_t;

/* The following type designates a signed integer type with the property that any valid pointer to void can be converted to this type, 
   then converted back to a pointer to void, and the result will compare equal to the original pointer: intptr_t */
typedef signed int intptr_t;
/* The following type designates an unsigned integer type with the property that any valid pointer to void can be converted to this type, 
   then converted back to a pointer to void, and the result will compare equal to the original pointer: uintptr_t */
typedef unsigned int uintptr_t;

        /* LIMIT MACROS */
#define INT8_MIN    (-0x7f - 1)
#define INT16_MIN   (-0x7fff - 1)
#define INT32_MIN   (-0x7fffffff - 1)
#define INT64_MIN   (-0x7fffffffffffffff - 1)

#define INT8_MAX    (0x7f)
#define INT16_MAX   (0x7fff)
#define INT32_MAX   (0x7fffffff)
#define INT64_MAX   (0x7fffffffffffffff)



#endif // __stdint_h

