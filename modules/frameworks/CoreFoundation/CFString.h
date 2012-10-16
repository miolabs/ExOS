//
//  CFString.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 12/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//


#ifndef __CFSTRING_H__
#define __CFSTRING_H__

#include "CFBase.h"
#include "CFData.h"
//#include "CFRuntime.h"

typedef struct __CFString
{
    CFBase _base;
    char *buffer;
    UInt8 len;
    UInt8 size;
    bool locked;
    
}CFString;

typedef CFString * CFStringRef;

#define CFSTR(cStr)  __CFStringMakeConstantString(cStr)

CFStringRef  CFStringCreateMutable(void);
CFStringRef  CFStringCreateMutableWithLength(unsigned int len);
CFStringRef  CFStringCreateMutableWithCString(const char *cStr);
CFStringRef  CFStringCreateMutableWithBytes(UInt8 *bytes, unsigned int len);

void CFStringAppendBytes(CFStringRef string, UInt8 bytes[], unsigned int len);
void CFStringAppendCString(CFStringRef string, const char *cStr);

UInt16 CFStringGetLength(CFStringRef string);
void CFStringGetCString(CFStringRef string, char buffer[]);

bool CFStringIsEqualToString(CFStringRef str1, CFStringRef str2);
void CFStringEncodeInData(CFStringRef string, CFDataRef data);

// Private Functions - Do not use!!!
CFStringRef __CFStringMakeConstantString(const char *cStr);
void __CFStringRegisterObject(void);

#endif
