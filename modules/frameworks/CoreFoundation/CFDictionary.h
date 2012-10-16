//
//  CFDictionary.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 10/10/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFDictionary_h
#define EmbeddedCoreFoundation_CFDictionary_h

#include "CFBase.h"
#include "CFArray.h"
#include "CFString.h"
//#include "CFRuntime.h"

typedef struct CFDictionary
{
    CFBase _base;
    CFArrayRef items;
    
}CFDictionary;

typedef CFDictionary *CFDictionaryRef;

CFDictionaryRef CFDictionaryCreateMutable(void);

void CFDictionarySetObjectForKey(CFDictionaryRef dic, CFTypeRef object, CFStringRef key);
CFTypeRef CFDictionaryGetObjectForKey(CFDictionaryRef dic, CFStringRef key);
void CFDictionaryRemoveObjectForKey(CFDictionaryRef dic, CFStringRef key);
void CFDictionaryRemoveAllObjects(CFDictionaryRef dic, CFTypeRef object, CFStringRef key);
UInt16 CFDictionaryGetCount(CFDictionaryRef dic);
void CFDictionaryAddEntriesFromDictionary(CFDictionaryRef dic, CFDictionaryRef otherDictionary);

// Private
void __CFDictionaryRegisterObject(void);

typedef struct CFDictionaryItem
{
    CFBase _base;
    CFTypeRef object;
    CFStringRef key;
    
}CFDictionaryItem;

#endif
