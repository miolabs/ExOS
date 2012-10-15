//
//  CFData.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 18/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFData_h
#define EmbeddedCoreFoundation_CFData_h

#include "CFBase.h"

typedef struct CFData
{
    CFBase _base;
    UInt8   *data;
    UInt16  len;
    UInt16  size;
    
}CFData;

typedef CFData *CFDataRef;

CFDataRef CFDataCreateMutable(void);
CFDataRef CFDataCreateMutableWithBytes(void *bytes, UInt16 len);

void CFDataAppendBytes(CFDataRef data, void *bytes, UInt16 len);
void CFDataAppendData(CFDataRef data, CFDataRef newData);

UInt16 CFDataGetLength(CFDataRef data);
void CFDataGetBytes(CFDataRef data, void *bytes, CFRange range);
void CFDataGetByteAtIndex(CFDataRef data, void *byte, UInt16 index);
CFDataRef CFDataGetSubdata(CFDataRef data, CFRange range); 
CFDataRef CFDataGetSubdataToIndex(CFDataRef data, UInt16 index); 
CFDataRef CFDataGetSubdataFromIndex(CFDataRef data, UInt16 index); 
void CFDataRemoveToIndex(CFDataRef data, UInt16 length);
void CFDataRemoveAll(CFDataRef data);

void CFDataReplaceBytesAtRange(CFDataRef data, void *byte, CFRange range);

// Private
void __CFDataRegisterObject(void);

#endif
