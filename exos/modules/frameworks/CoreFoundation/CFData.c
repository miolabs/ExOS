//
//  CFData.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 18/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#include "CFData.h"
//#include "CFRuntime.h"

#pragma mark Private Functions Prototype
void __CFDataAddBytes(CFDataRef data, UInt8 bytes[], UInt16 len);
void __CFDataDeallocCallback(CFTypeRef object);
//void __CFDataEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFDataDecodeCallback(CFCoderRef coder);

CFAllocatorsCallbacks mCFDataDefaultAllocators = { NULL, __CFDataDeallocCallback};
//CFRuntimeCallbacks mCFDataDefaultCoders = {__CFDataEncodeCallback, __CFDataDecodeCallback};

//void __CFDataRegisterObject(void)
//{
//    CFRuntimeRegisterObjectType(kCFDataType, &mCFDataDefaultCoders);       
//}

#pragma mark - Public Functions

CFDataRef CFDataCreateMutable(void)
{
    return CFDataCreateMutableWithBytes(NULL, 0);
}

CFDataRef CFDataCreateMutableWithBytes(void *bytes, UInt16 len)
{
    CFDataRef data = (CFDataRef)CFAllocatorCreate(kCFDataType, sizeof(CFData), &mCFDataDefaultAllocators);
    if (data != NULL)
    {
        data->data = (UInt8 *)malloc(len + 10);
        data->size = len + 10;
        data->len = 0;        
        __CFDataAddBytes(data, bytes, len);
    }
    
    return data;
}

void CFDataAppendBytes(CFDataRef data, void *bytes, UInt16 len)
{
    if (data == NULL)
        return;
    
   // if (data->len + len > kCFDataMaxNumberOfBytesPerObject)
   //     return;
    
    __CFDataAddBytes(data, bytes, len);
    
}

void CFDataAppendData(CFDataRef data, CFDataRef newData)
{
    CFDataAppendBytes(data, newData->data, newData->len);
}

UInt16 CFDataGetLength(CFDataRef data)
{
    if (data == NULL)
        return 0;
    
    return data->len;
}

void CFDataGetBytes(CFDataRef data, void *bytes, CFRange range)
{
    UInt8 count = 0;
    UInt8 *pointer = bytes;
    
    if (range.length + range.location > data->len)
        return;
    
    for (count = 0; count < range.length; count++) 
    {
        *pointer = data->data[count + range.location];
        pointer++;
    }
}

void CFDataGetByteAtIndex(CFDataRef data, void *byte, UInt16 index)
{
    CFRange range = {index, 1};
    CFDataGetBytes(data, byte, range);
}

CFDataRef CFDataGetSubdata(CFDataRef data, CFRange range)
{    
    if (data == NULL)
        return NULL;
    
    if (range.location + range.length > data->len)
        return NULL;
    
    CFDataRef buffer = CFDataCreateMutable();
    CFDataAppendBytes(buffer, data->data + range.location, range.length);
    return buffer;
}

CFDataRef CFDataGetSubdataToIndex(CFDataRef data, UInt16 index)
{
    CFRange range = {0, index};
    return CFDataGetSubdata(data, range);
}

CFDataRef CFDataGetSubdataFromIndex(CFDataRef data, UInt16 index)
{    
    UInt16 len = CFDataGetLength(data);
    CFRange range = {index, len - index};
    return CFDataGetSubdata(data, range);    
}

void CFDataRemoveToIndex(CFDataRef data, UInt16 length)
{
    UInt16 newLength = CFDataGetLength(data) - length;
    if (newLength > 0)
    {
        // Optimization. We copy teh offset data in teh data byte per byte
        memcpy(data->data, data->data + length, newLength);
        data->len = newLength;
    }
    else 
        CFDataRemoveAll(data);
}

void CFDataRemoveAll(CFDataRef data)
{
    // Optimization. We dont need to free memory. We can overwrite.
    data->len = 0;
}

void CFDataReplaceBytesAtRange(CFDataRef data, void *bytes, CFRange range)
{
    if (range.location + range.length > data->len)
        return;
    
    memcpy(data->data + range.location, bytes, range.length);
}

#pragma mark - Private Functions and Callbacks

void __CFDataAddBytes(CFDataRef data, UInt8 bytes[], UInt16 len)
{
    if (len < 1)
        return;
    
    if (bytes == NULL)
        return;
    
    if (data->len + len < data->size)
    {    
        memcpy(data->data + data->len, bytes, len);
        data->len += len;
    }
    else
    {
        UInt8 *buffer = (UInt8 *)malloc(data->len + len + 10);
        if (buffer == NULL)
            return;
        // Copy old data
        memcpy(buffer, data->data, data->len);
        // Append new data
        memcpy(buffer + data->len, bytes, len);
        free(data->data);
        data->data = buffer;
        data->len += len;
        data->size = data->len + 10;
    }
}

#pragma mark - Runtime Callbacks

void __CFDataDeallocCallback(CFTypeRef object)
{
    CFDataRef data = (CFDataRef)object;
    free(data->data);
    data->len = 0;
    data->size = 0;   
}

//void __CFDataEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFDataRef data = (CFDataRef)object;
//    UInt16 len = CFDataGetLength(data);
//    CFDataAppendBytes(coder->data, &len, 2);
//    CFDataAppendBytes(coder->data, data->data, len);
//}
//
//CFTypeRef __CFDataDecodeCallback(CFCoderRef coder)
//{
//    if (coder == NULL)
//        return NULL;
//
//    UInt16 len;
//    CFDataRef data = NULL;
//    
//    CFRange range;
//    range.location = coder->index;
//    range.length = 2;
//    CFDataGetBytes(coder->data, &len, range);
//    coder->index += 2;
//    
//    if (len > 0)
//    {
//        CFRange range = {coder->index, len};
//        data = CFDataGetSubdata(coder->data, range);
//        coder->index += len;
//    }
//    
//    return data;    
//}

