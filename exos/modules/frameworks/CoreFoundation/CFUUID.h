//
//  CFUUID.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 22/11/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFUUID_h
#define EmbeddedCoreFoundation_CFUUID_h

#include "CFBase.h"
#include "CFString.h"

typedef struct CFUUID
{
    CFBase _base;
    UInt8   uuid[16];
    
}CFUUID;

typedef CFUUID * CFUUIDRef;

CFUUIDRef CFUUIDCreate(void);
CFUUIDRef CFUUIDGenerate(void);
CFUUIDRef CFUUIDCreateWithBytes(UInt8 bytes[16]);
CFUUIDRef CFUUIDCreateWithByte(UInt8 byte);
CFUUIDRef CFUUIDCreateWith16Bytes(UInt8 byte0, UInt8 byte1, UInt8 byte2, UInt8 byte3,
                                    UInt8 byte4, UInt8 byte5, UInt8 byte6, UInt8 byte7,
                                    UInt8 byte8, UInt8 byte9, UInt8 byte10, UInt8 byte11,
                                    UInt8 byte12, UInt8 byte13, UInt8 byte14, UInt8 byte15);
CFUUIDRef CFUUIDCreateWithCString(char uuidString[]);

bool CFUUIDIsEqualToUUID(CFUUIDRef uuid1, CFUUIDRef uuid2);
bool CFUUIDIsZero(CFUUIDRef uuid);
CFStringRef CFUUIDGetString(CFUUIDRef uuid);

// Private
void __CFUUIDRegisterObject(void);

#endif
