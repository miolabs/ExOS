//
//  CFUUID.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 22/11/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#include "CFUUID.h"
//#include "CFCoder.h"
//#include "CFRuntime.h"


//void __CFUUIDEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFUUIDDecodeCallback(CFCoderRef coder);

//CFRuntimeCallbacks mCFUUIDDefaultCoders = {__CFUUIDEncodeCallback, __CFUUIDDecodeCallback};

//void __CFUUIDRegisterObject(void)
//{
//    CFRuntimeRegisterObjectType(kCFUUIDType, &mCFUUIDDefaultCoders);   
//}

CFUUIDRef CFUUIDCreate(void)
{
    CFUUIDRef uuid = (CFUUIDRef)CFAllocatorCreate(kCFUUIDType, sizeof(CFUUID), NULL);
    if (uuid != NULL)
    {
        memset(uuid->uuid, 0, 16);
    }
    return uuid;
}

CFUUIDRef CFUUIDGenerate(void)
{
    CFUUIDRef uuid = (CFUUIDRef)CFAllocatorCreate(kCFUUIDType, sizeof(CFUUID), NULL);
    if (uuid != NULL)
    {
        UInt8 count = 0;
        for (count = 0; count < 16; count++)
        {
            UInt8 byte = 0;
            byte = rand() % 255;
            uuid->uuid[count] = byte;
        }
    }
    return uuid;    
}

CFUUIDRef CFUUIDCreateWithBytes(UInt8 bytes[16])
{
    CFUUIDRef uuid = CFUUIDCreate();
    if (uuid != NULL)
    {
        memcpy(uuid->uuid, bytes, 16);
    }
    return uuid;
}

CFUUIDRef CFUUIDCreateWithByte(UInt8 byte)
{
    CFUUIDRef uuid = CFUUIDCreate();
    if (uuid != NULL)
    {
        memset(uuid->uuid, byte, 16);
    }
    return uuid;
}

CFUUIDRef CFUUIDCreateWith16Bytes(UInt8 byte0, UInt8 byte1, UInt8 byte2, UInt8 byte3,
                                    UInt8 byte4, UInt8 byte5, UInt8 byte6, UInt8 byte7,
                                    UInt8 byte8, UInt8 byte9, UInt8 byte10, UInt8 byte11,
                                    UInt8 byte12, UInt8 byte13, UInt8 byte14, UInt8 byte15)
{
    UInt8 bytes[16];
    bytes[0] = byte0;
    bytes[1] = byte1;
    bytes[2] = byte2;
    bytes[3] = byte3;
    bytes[4] = byte4;
    bytes[5] = byte5;
    bytes[6] = byte6;
    bytes[7] = byte7;
    bytes[8] = byte8;
    bytes[9] = byte9;
    bytes[10] = byte10;
    bytes[11] = byte11;
    bytes[12] = byte12;
    bytes[13] = byte13;
    bytes[14] = byte14;
    bytes[15] = byte15;

    return CFUUIDCreateWithBytes(bytes);
}

CFUUIDRef CFUUIDCreateWithCString(char uuidString[])
{
    if (uuidString == NULL)
        return NULL;
    
    CFUUIDRef uuid = CFUUIDCreate();
    if (uuid != NULL)
    {
        unsigned char whole_byte;
        char byte_chars[3] = {'\0','\0','\0'};
        int i;
        for (i=0; i < 16; i++) {
            byte_chars[0] = uuidString[i * 2];
            byte_chars[1] = uuidString[i * 2 + 1];
            whole_byte = strtol(byte_chars, NULL, 16);
            uuid->uuid[i] = whole_byte;
        }
    }
    return uuid;
}

CFStringRef CFUUIDGetString(CFUUIDRef uuid)
{
    if (uuid == NULL)
        return NULL;
    
    CFStringRef string = CFStringCreateMutable();
    
    char buffer[3];
    
    UInt8 count = 0;
    for (count = 0; count < 16; count++)
    {
        sprintf(buffer, "%02X", uuid->uuid[count]);
        CFStringAppendCString(string, buffer);        
    }
    
    return string;
}

bool CFUUIDIsEqualToUUID(CFUUIDRef uuid1, CFUUIDRef uuid2)
{
    if (uuid1 == NULL || uuid2 == NULL)
        return FALSE;
    
    if (memcmp(uuid1->uuid, uuid2->uuid, 16) == 0)
        return TRUE;
    
    return FALSE;
}

bool CFUUIDIsZero(CFUUIDRef uuid)
{
    if (uuid == NULL)
        return FALSE;
    
    UInt8 count = 0;
    for (count = 0; count < 16; count++)
    {
        if (uuid->uuid[count] != 0)
            return FALSE;
    }
    
    return TRUE;
}

#pragma mark - Private

//void __CFUUIDEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFUUIDRef uuid = (CFUUIDRef)object;
//    UInt8 count = 0;
//    for (count = 0; count < 16; count++) 
//        CFCoderEncodeUInt8(coder, uuid->uuid[count]);
//}
//
//CFTypeRef __CFUUIDDecodeCallback(CFCoderRef coder)
//{
//    CFUUIDRef uuid = CFUUIDCreate();
//    UInt8 count = 0;
//    for (count = 0; count < 16; count++) 
//        uuid->uuid[count] = CFCoderDecodeUInt8(coder);
//    
//    return uuid;
//}
