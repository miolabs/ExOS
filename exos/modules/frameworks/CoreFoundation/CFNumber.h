//
//  CFNumber.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 01/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFNumber_h
#define EmbeddedCoreFoundation_CFNumber_h

#include "CFBase.h"

typedef struct __CFNumber
{
    CFBase _base;
    UInt8 *buffer;
    UInt8 size;
    
}CFNumber;

typedef CFNumber * CFNumberRef;

CFNumberRef CFNumberCreate(void);
CFNumberRef CFNumberCreateWithUInt8(UInt8 value);
CFNumberRef CFNumberCreateWithUInt16(UInt16 value);

void CFNumberSetUInt8(CFNumberRef number, UInt8 value);
UInt8 CFNumberGetUInt8(CFNumberRef number);
void CFNumberSetUInt16(CFNumberRef number, UInt16 value);
UInt16 CFNumberGetUInt16(CFNumberRef number);


// Private
void __CFNumberRegisterObject(void);

#endif
