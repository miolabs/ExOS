//
//  CFString.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 13/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#include "CFString.h"
#include <string.h>


// Callbacks
void __CFStringDeallocCallback(CFTypeRef object);
//void __CFStringEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFStringDecodeCallback(CFCoderRef coder);

CFAllocatorsCallbacks mCFStringDefaultAllocators = { NULL, __CFStringDeallocCallback};
//CFRuntimeCallbacks mCFStringDefaultCoders = {__CFStringEncodeCallback, __CFStringDecodeCallback};

//void __CFStringRegisterObject(void)
//{
//     CFRuntimeRegisterObjectType(kCFStringType, &mCFStringDefaultCoders);   
//}

// Public Functions
CFStringRef CFStringCreateMutable(void)
{
    return CFStringCreateMutableWithLength(15);
}

CFStringRef  CFStringCreateMutableWithLength(unsigned int len)
{
    CFStringRef string = (CFStringRef)CFAllocatorCreate(kCFStringType, sizeof(CFString), &mCFStringDefaultAllocators);
    if (string != NULL)
    {
        string->buffer = NULL;
        string->len = 0;
        string->buffer = (char *)malloc(len);        
        string->size = len;
        string->locked = FALSE;
    }
    
    return string;    
}

CFStringRef CFStringCreateMutableWithCString(const char *cStr)
{
    unsigned int len = (unsigned int)strlen(cStr);
    
    CFStringRef string = CFStringCreateMutableWithLength(len);
    if (string != NULL)
        CFStringAppendCString(string, cStr);
    
    return string;
}

CFStringRef  CFStringCreateMutableWithBytes(UInt8 *bytes, unsigned int len)
{
    CFStringRef string = CFStringCreateMutableWithLength(len);
    if (string != NULL)
        CFStringAppendBytes(string, bytes, len);
    
    return string;
}

void CFStringAppendBytes(CFStringRef string, UInt8 bytes[], unsigned int len)
{
	if (!string)
		return;
	
    if (string->len + len < string->size)
    {    
        char *pointer = string->buffer + string->len;
        int count;
        for (count = 0; count < len; count++)
        {
            *pointer = bytes[count];
            pointer++;
            string->len++;
        }
    }
    else 
    {
        UInt16 count = 0;
        char *oldBuffer = string->buffer;
        char *oldBufferPointer = oldBuffer;
        
        char *buffer = (char *)malloc(len + string->len + 10);
        if (buffer == NULL)
            return;
        
        char *pointer = buffer;            
        // Copy old content
        for (count = 0; count < string->len; count++)
        {
            *pointer = *oldBufferPointer;
            pointer++;
            oldBufferPointer++;
        }

        // Copy new content
        for (count = 0; count < len; count++)
        {
            *pointer = bytes[count];
            pointer++;
        }           
        
        string->len = len + string->len;
        string->size = string->len + 10;        
        string->buffer = buffer;    

        free(oldBuffer);
    }    
}

void CFStringAppendCString(CFStringRef string, const char *cStr)
{
    CFStringAppendBytes(string, (UInt8 *)cStr, (unsigned int)strlen(cStr)); 
}

UInt16 CFStringGetLength(CFStringRef string)
{
    return string->len;
}

void CFStringGetCString(CFStringRef string, char buffer[])
{
    UInt8 count = 0;
    if (string == NULL)
        return;
    
    for (count = 0; count < string->len; count++)
        buffer[count] = string->buffer[count];
    buffer[count] = '\0';
    
}

bool CFStringIsEqualToString(CFStringRef str1, CFStringRef str2)
{
	if (str1 == NULL && str2 == NULL) 
		return TRUE;
	
	if (str1 == NULL || str2 == NULL)
		return FALSE;
	
    if (str1->len != str2->len)
        return FALSE;
    
    if (strncmp(str1->buffer, str2->buffer, str1->len) == 0)
        return TRUE;
    
    return FALSE;
}

void CFStringEncodeInData(CFStringRef string, CFDataRef data)
{
    UInt8 len = CFStringGetLength(string);
    CFDataAppendBytes(data, &len, 1);
    CFDataAppendBytes(data, string->buffer, len);
}

#pragma mark - Runtime callbacks

void __CFStringDeallocCallback(CFTypeRef object)
{
    CFStringRef string = (CFStringRef)object;
    if (!string->locked)
        free(string->buffer);
    string->len = 0;
    string->size = 0;
}

//void __CFStringEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFStringRef string = (CFStringRef)object;
//    CFStringEncodeInData(string, coder->data);
//}
//
//CFTypeRef __CFStringDecodeCallback(CFCoderRef coder)
//{
//    UInt8 len;
//    char buffer[kCFStringMaxNumberOfChars];
//    CFStringRef string = NULL;
//    
//    CFRange range = {coder->index, 1};
//    CFDataGetBytes(coder->data, &len, range);
//    coder->index++;
//    
//    if (len > 0)
//    {
//        CFRange range = {coder->index, len};
//        CFDataGetBytes(coder->data, buffer, range);
//        string = CFStringCreateMutable();
//        CFStringAppendBytes(string, (UInt8 *)buffer, len);
//        coder->index += len;
//        // TODO Implement Autorelease Pool
//        //CFRelease(string);
//    }
//    
//    return string;
//}

#pragma mark - Private

CFStringRef __CFStringMakeConstantString(const char *cStr)
{
    int len = (int)strlen(cStr);
    
    CFStringRef string = (CFStringRef)CFAllocatorCreate(kCFStringType, sizeof(CFString), &mCFStringDefaultAllocators);
    if (string != NULL)
    {
        string->buffer = NULL;
        string->len = len;
        string->buffer = (char *)cStr;
        string->size = len;
        string->locked = TRUE;
    }
    
    return string;
}
