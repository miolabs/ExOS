//
//  CFBase.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 12/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#if !defined(__COREFOUNDATION_CFBASE__)
#define __COREFOUNDATION_CFBASE__ 1

//#include <TargetConditionals.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define TRUE true
#define FALSE false

typedef unsigned char           Boolean;
typedef unsigned char           UInt8;
typedef signed char             SInt8;
typedef unsigned short          UInt16;
typedef signed short            SInt16;
typedef unsigned int            UInt32;
typedef signed int              SInt32;
typedef uint64_t                UInt64;
typedef int64_t                 SInt64;
typedef SInt32                  OSStatus;
typedef float                   Float32;
typedef double                  Float64;
typedef unsigned short          UniChar;
typedef unsigned long           UniCharCount;
typedef unsigned char *         StringPtr;
typedef const unsigned char *   ConstStringPtr;
typedef unsigned char           Str255[256];
typedef const unsigned char *   ConstStr255Param;
typedef SInt16                  OSErr;
typedef SInt16                  RegionCode;
typedef SInt16                  LangCode;
typedef SInt16                  ScriptCode;
typedef UInt32                  FourCharCode;
typedef FourCharCode            OSType;
typedef UInt8                   Byte;
typedef SInt8                   SignedByte;


// types
#define kCFNullType            "NUL"
#define kCFStringType          "STR"
#define kCFNumberType          "NUM"
#define kCFDataType            "DAT"
#define kCFArrayType           "ARR"
#define kCFDictionaryType      "DIC"
#define kCFCoderType           "CDR"
#define kCFUUIDType            "UID"
#define kCFRunLoopTimerType    "RLT"
#define kCFRunLoopSourceType   "RLS"
#define kCFBusType             "BUS"

#define kCFCoderTypeLen        3

#define kCFStringMaxNumberOfChars 100
#define kCFDataMaxNumberOfBytesPerObject 2048
#define kCFArraMaxNumberOfItems 500
#define kCFRuntimePointerPoolMaxNumberOfPointers 1000

// Range

typedef struct CFRange
{
    UInt16 location;
    UInt16 length;
    
}CFRange;
/*
#define CFRangeMake(loc, len) __CFRangeMake(loc, len)

CFRange __CFRangeMake(UInt16 loc, UInt16 len);
*/

// Base type
typedef void * CFTypeRef;

// Base
typedef void (*CFAllocCallback)(CFTypeRef object);
typedef void (*CFDeallocCallback)(CFTypeRef object);

typedef struct CFAllocatorsCallbacks
{
    CFAllocCallback allocCallback;
    CFDeallocCallback deallocCallback;
    
}CFAllocatorsCallbacks;

typedef struct __CFBase
{
    char *type;
    UInt8 rc;
    CFAllocatorsCallbacks *callbacks;    
    
}CFBase;

bool CFBaseIsType(CFTypeRef object, char *type);

// Allocators
CFTypeRef CFAllocatorCreate(char *type, UInt16 size, CFAllocatorsCallbacks *callbacks);
void CFAllocatorDestroy(CFTypeRef object);
void CFRetain(CFTypeRef object);
void CFRelease(CFTypeRef object);

// only for debug
int CFBaseGetRetainCounter(void);
void CFBaseResetRetainCounter(void);

#endif
