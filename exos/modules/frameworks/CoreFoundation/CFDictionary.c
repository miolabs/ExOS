//
//  CFDictionary.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 10/10/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#include "CFDictionary.h"

#pragma mark - Dictonary Item (Private)

#define kCFDictionaryItemType "DIT"

void __CFDictionaryItemDeallocCallback(CFTypeRef object);
//void __CFDictionaryItemEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFDictionaryItemDecodeCallback(CFCoderRef coder);

//CFRuntimeCallbacks mCFDictionaryItemDefaultCoders = {__CFDictionaryItemEncodeCallback, __CFDictionaryItemDecodeCallback};
CFAllocatorsCallbacks mCFDictionaryItemDefaultAllocators = { NULL, __CFDictionaryItemDeallocCallback};

CFDictionaryItem *__CFDictionaryItemCreate(void);
CFDictionaryItem *__CFDictionaryItemCreateWitObjectAndKey(CFTypeRef object, CFStringRef key);

CFDictionaryItem *__CFDictionaryItemCreate(void)
{
    CFDictionaryItem *item = (CFDictionaryItem *)CFAllocatorCreate(kCFDictionaryItemType, sizeof(CFDictionaryItem), &mCFDictionaryItemDefaultAllocators);
    if (item != NULL)
    {
        item->object = NULL;
        item->key = NULL;
    }
    
    return item;
}

CFDictionaryItem *__CFDictionaryItemCreateWitObjectAndKey(CFTypeRef object, CFStringRef key)
{
    CFDictionaryItem *item = __CFDictionaryItemCreate();
    if (item != NULL)
    {
        item->object = object;
        CFRetain(object);
        item->key = key;
        CFRetain(key);
    }
    
    return item;
}

#pragma mark - Runtime callbacks for CFDictionaryItem

void __CFDictionaryItemDeallocCallback(CFTypeRef object)
{
    CFDictionaryItem *item = (CFDictionaryItem *)object;
    CFRelease(item->object);
    CFRelease(item->key);
}

//void __CFDictionaryItemEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFDictionaryItem *item = (CFDictionaryItem *)object;
//    CFCoderEncodeObject(coder, item->key);
//    CFCoderEncodeObject(coder, item->object);
//}
//
//CFTypeRef __CFDictionaryItemDecodeCallback(CFCoderRef coder)
//{
//    CFDictionaryItem *item = __CFDictionaryItemCreate();
//    item->key = CFCoderDecodeObject(coder);
//    item->object = CFCoderDecodeObject(coder);
//    
//    return item;
//}

#pragma mark - Dictonary

void __CFDictionaryDeallocCallback(CFTypeRef object);
//void __CFDictionaryEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFDictionaryDecodeCallback(CFCoderRef coder);

CFAllocatorsCallbacks mCFDictionaryDefaultCallbacks = {NULL, __CFDictionaryDeallocCallback};
//CFRuntimeCallbacks mCFDictionaryDefaultCoders = {__CFDictionaryEncodeCallback, __CFDictionaryDecodeCallback};

CFDictionaryItem *__CFDictionaryGetItemForKey(CFDictionaryRef dic, CFStringRef key);

//void __CFDictionaryRegisterObject(void)
//{
//    CFRuntimeRegisterObjectType(kCFDictionaryType, &mCFDictionaryDefaultCoders);   
//    CFRuntimeRegisterObjectType(kCFDictionaryItemType, &mCFDictionaryItemDefaultCoders);   
//}

CFDictionaryRef CFDictionaryCreateMutable(void)
{
    CFDictionaryRef dic = (CFDictionaryRef)CFAllocatorCreate(kCFDictionaryType, sizeof(CFDictionary), &mCFDictionaryDefaultCallbacks);
    if (dic != NULL)
    {
        dic->items = NULL;
    }
    return dic;
}

CFDictionaryItem *__CFDictionaryGetItemForKey(CFDictionaryRef dic, CFStringRef key)
{
    if (dic == NULL || key == NULL)
        return NULL;

    CFArrayIterator iterator = CFArrayGetIterator(dic->items);
    CFDictionaryItem *item = NULL;
    bool foundKey = FALSE;    
    
    while (iterator != NULL) 
    {
        item = (CFDictionaryItem *)iterator->object;
        if (CFStringIsEqualToString(item->key, key))
        {
            foundKey = TRUE;            
            iterator = NULL;
        }
        else
            iterator = CFArrayMoveForward(iterator);
    }
    
    if (foundKey)
        return item;
    
    return NULL;
}

void CFDictionarySetObjectForKey(CFDictionaryRef dic, CFTypeRef object, CFStringRef key)
{
    CFDictionaryItem *item = NULL;
    
    if (dic->items == NULL)
        dic->items = CFArrayCreateMutable();
            
    item = __CFDictionaryGetItemForKey(dic, key);
    
    if (item != NULL)
    {
        CFRelease(item->object);
        item->object = object;
        CFRetain(object);
    }
    else
    {
        item = __CFDictionaryItemCreateWitObjectAndKey(object, key);
        CFArrayAddObject(dic->items, item);
		CFRelease(item);
    }
}

CFTypeRef CFDictionaryGetObjectForKey(CFDictionaryRef dic, CFStringRef key)
{
    CFDictionaryItem *item = __CFDictionaryGetItemForKey(dic, key);
    
    if (item != NULL)
        return item->object;
    
    return NULL;    
}

void CFDictionaryRemoveObjectForKey(CFDictionaryRef dic, CFStringRef key)
{
    CFDictionaryItem *item = __CFDictionaryGetItemForKey(dic, key);
    if (item != NULL)
        CFArrayRemoveObject(dic->items, item);
}

void CFDictionaryRemoveAllObjects(CFDictionaryRef dic, CFTypeRef object, CFStringRef key)
{
    CFArrayRemoveAllItems(dic->items);
}

UInt16 CFDictionaryGetCount(CFDictionaryRef dic)
{
	if (dic == NULL)
		return 0;
	
	return CFArrayGetCount(dic->items);
}

void CFDictionaryAddEntriesFromDictionary(CFDictionaryRef dic, CFDictionaryRef otherDictionary)
{
	CFArrayIterator iterator = CFArrayGetIterator(otherDictionary->items);
    CFDictionaryItem *newItem = NULL;
	
    while (iterator != NULL)
    {
        newItem = (CFDictionaryItem *)iterator->object;
		CFDictionarySetObjectForKey(dic, newItem->object, newItem->key);
		
		iterator = CFArrayMoveForward(iterator);
    }
}

#pragma mark - Runtime callbacks for CFDictionary

void __CFDictionaryDeallocCallback(CFTypeRef object)
{
    CFDictionaryRef dic = (CFDictionaryRef)object;
    CFRelease(dic->items);
}

//void __CFDictionaryEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFDictionaryRef dic = (CFDictionaryRef)object;
//    CFCoderEncodeObject(coder, dic->items);
//}
//
//CFTypeRef __CFDictionaryDecodeCallback(CFCoderRef coder)
//{
//    CFDictionaryRef dic = CFDictionaryCreateMutable();
//    dic->items = CFCoderDecodeObject(coder);
//    
//    return dic;
//}
