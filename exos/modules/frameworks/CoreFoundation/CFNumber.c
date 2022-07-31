//
//  CFNumber.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 01/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#include "CFNumber.h"
//#include "CFRuntime.h"


// Callbacks
void __CFNumberDeallocCallback(CFTypeRef object);
//void __CFNumberEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFNumberDecodeCallback(CFCoderRef coder);

CFAllocatorsCallbacks mCFNumberDefaultAllocators = { NULL, __CFNumberDeallocCallback};
//CFRuntimeCallbacks mCFNumberDefaultCoders = {__CFNumberEncodeCallback, __CFNumberDecodeCallback};


//void __CFNumberRegisterObject(void)
//{
//    CFRuntimeRegisterObjectType(kCFNumberType, &mCFNumberDefaultCoders);   
//}

// Public Functions
CFNumberRef CFNumberCreate(void)
{
    CFNumberRef number = (CFNumberRef)CFAllocatorCreate(kCFNumberType, sizeof(CFNumber), &mCFNumberDefaultAllocators);
    if (number != NULL)
    {
        number->buffer = NULL;
        number->size = 0;
    }
    
    return number;
}

CFNumberRef CFNumberCreateWithUInt8(UInt8 value)
{
    CFNumberRef number = CFNumberCreate();
    if (number != NULL)
        CFNumberSetUInt8(number, value);
    
    return number;
}

CFNumberRef CFNumberCreateWithUInt16(UInt16 value)
{
    CFNumberRef number = CFNumberCreate();
    if (number != NULL)
        CFNumberSetUInt16(number, value);
    
    return number;    
}

void __CFNumberSetValue(CFNumberRef number, UInt8 *value, UInt8 size);
void __CFNumberSetValue(CFNumberRef number, UInt8 *value, UInt8 size)
{
    if (number == NULL)
        return;
    
    if (number->buffer != NULL)
        free(number->buffer);
    
    number->buffer = (UInt8 *)malloc(size);    
    memcpy(number->buffer, value, size);
	number->size = size;
}

void CFNumberSetUInt8(CFNumberRef number, UInt8 value)
{
    __CFNumberSetValue(number, &value, 1);
}

UInt8 CFNumberGetUInt8(CFNumberRef number)
{
    if (number == NULL)
        return 0;
    
    UInt8 value = *(number->buffer + number->size - 1);
    return value;
}

void CFNumberSetUInt16(CFNumberRef number, UInt16 value)
{
    UInt8 bytes[2];
    
    bytes[0] = value >> 8;
    bytes[1] = (UInt8)value;
    __CFNumberSetValue(number, bytes, 2);    
}

UInt16 CFNumberGetUInt16(CFNumberRef number)
{
    UInt16 value = 0;
    
    if (number->size == 1)
        value = *(number->buffer);
    else if (number->size > 1)
    {
        value = *(number->buffer + number->size - 2);
        value = value << 8;
        value += *(number->buffer + number->size - 1);
    }
    
    return value;
}

#pragma mark - Runtime callbacks

void __CFNumberDeallocCallback(CFTypeRef object)
{
    CFNumberRef number = (CFNumberRef)object;
    if (number->size > 0)
        free(number->buffer);
    number->size = 0;
}

//void __CFNumberEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFNumberRef number = (CFNumberRef)object;
//    UInt8 count = 0;
//    
//    CFCoderEncodeUInt8(coder, number->size);
//    for (count = 0; count < number->size; count++) 
//    {
//        UInt8 buffer = *(number->buffer + count);
//        CFCoderEncodeUInt8(coder, buffer);
//    }
//}
//
//CFTypeRef __CFNumberDecodeCallback(CFCoderRef coder)
//{
//    CFNumberRef number = CFNumberCreate();
//    number->size = CFCoderDecodeUInt8(coder);
//    
//    number->buffer = (UInt8 *)malloc(number->size);
//    UInt8 count = 0;
//    
//    for (count = 0; count < number->size; count++)
//        *(number->buffer + count) = CFCoderDecodeUInt8(coder);
//    
//    return number;
//}
