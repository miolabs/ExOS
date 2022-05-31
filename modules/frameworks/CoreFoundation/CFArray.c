//
//  CFArray.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 18/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#include "CFArray.h"

// Private
void __CFArrayInit(CFArrayRef array);
CFArrayIterator __CFArrayGetItemAtIndex(CFArrayRef array, unsigned int index);
// Callbacks
void __CFArrayDeallocCallback(CFTypeRef object);
//void __CFArrayEncodeCallback(CFTypeRef object, CFCoderRef coder);
//CFTypeRef __CFArrayDecodeCallback(CFCoderRef coder);

CFAllocatorsCallbacks mCFArrayDefaultAllocators = { NULL, __CFArrayDeallocCallback};
//CFRuntimeCallbacks mCFArrayDefaultCoders = {__CFArrayEncodeCallback, __CFArrayDecodeCallback};

//void __CFArrayRegisterObject(void)
//{
//    CFRuntimeRegisterObjectType(kCFArrayType, &mCFArrayDefaultCoders);   
//}

#pragma mark - Public Functions

CFArrayRef CFArrayCreateMutable(void)
{
    CFArrayRef array = (CFArrayRef)CFAllocatorCreate(kCFArrayType, sizeof(CFArray), &mCFArrayDefaultAllocators);
    if (array != NULL)
    { 
        __CFArrayInit(array);
    }
    return array;

}

int CFArrayGetCount(CFArrayRef array)
{
    if (array == NULL)
        return 0;
        
    return array->count;
}

void CFArrayAddObject(CFArrayRef array, CFTypeRef object)
{
    if (array == NULL)
        return;
    
    if (array->count >= kCFArraMaxNumberOfItems)
        return;

    CFArrayIterator iterator = (CFArrayIterator)malloc(sizeof(__CFArrayItem));
    if (iterator == NULL)
        return;

    iterator->nextItem = NULL;
    iterator->object = object;
    iterator->prevItem = NULL;
    
    CFArrayAddIterator(array, iterator);
    CFRetain(object);
}


void CFArrayRemoveObject(CFArrayRef array, CFTypeRef object)
{
	if(object == NULL)
        return;
    
    if (array == NULL)
        return;
	
    CFArrayIterator item = CFArrayGetIterator(array);
    while (item != NULL) 
    {
        if (item->object == object)
        {
            CFRelease(item->object);
            CFArrayRemoveIterator(array, item);
            item = NULL;
        }    
        else
            item = CFArrayMoveForward(item);
    }
}

void CFArrayRemoveObjectAtIndex(CFArrayRef array, unsigned int index)
{
    if (array == NULL)
        return;
    
	CFArrayIterator item = __CFArrayGetItemAtIndex(array, index);
    CFRelease(item->object);
    CFArrayRemoveIterator(array, item);
}

void CFArrayRemoveAllItems(CFArrayRef array)
{
	if (array == NULL)
		return;
    
	CFArrayIterator del = CFArrayGetIterator(array);
    
	while(del != NULL)
    {
        CFRelease(del->object);
        del = CFArrayRemoveIterator(array, del);		
    }
    
	__CFArrayInit(array);
}


CFTypeRef CFArrayGetObjectAtIndex(CFArrayRef array, int index)
{
    CFTypeRef object = NULL;
    
    CFArrayIterator iterator = __CFArrayGetItemAtIndex(array, index);
    if (iterator != NULL)
        object = iterator->object;
    
    return object;
}

CFArrayIterator CFArrayGetIterator(CFArrayRef array)
{
    if (array == NULL)
        return NULL;

    return array->startItem;
}

void CFArrayAddIterator(CFArrayRef array, CFArrayIterator iterator)
{    
    if (array->endItem == NULL)
    {
        iterator->prevItem = NULL;
        array->startItem = iterator;
        array->endItem = iterator;
    }
    else
    {
        iterator->prevItem = array->endItem;
        array->endItem->nextItem = iterator;
        array->endItem = iterator;
    }
    array->count++;   
}

CFArrayIterator CFArrayRemoveIterator(CFArrayRef array, CFArrayIterator iterator)
{
    __CFArrayItem *prevItem = iterator->prevItem;
    __CFArrayItem *nextItem = iterator->nextItem;
    
    if(prevItem != NULL)
        prevItem->nextItem = nextItem;
    
    if(nextItem != NULL)
        nextItem->prevItem = prevItem;
    
    if(iterator == array->startItem)
        array->startItem = nextItem;
    
    if(iterator == array->endItem)
        array->endItem = prevItem;
    
    free(iterator);
    array->count--;

    return nextItem;
}

CFArrayIterator CFArrayMoveForward(CFArrayIterator iterator)
{
    if (iterator != NULL)
            return iterator->nextItem;
    
    return NULL;
}

CFArrayIterator CFArrayMoveBackwards(CFArrayIterator iterator)
{
    if (iterator != NULL)
            return iterator->prevItem;
    
	return NULL;
}

#pragma mark - Private Functions

void __CFArrayInit(CFArrayRef array)
{
    array->count = 0;    
    array->startItem = NULL;
    array->endItem = NULL;    
}

CFArrayIterator __CFArrayGetItemAtIndex(CFArrayRef array, unsigned int index)
{
    __CFArrayItem *item = NULL;
	int middle;
    
	if (array == NULL)
		return item;
    
	if(index >= array->count)
		return item;
    
	middle = array->count / 2;
    
	if(index < middle)
	{
		item = array->startItem;
		int count;
		for(count = 0; count < index; count++)
			item = item->nextItem;
	}
	else
	{
		item = array->endItem;
        int count;
		for(count = array->count - 1; count > index; count--)
			item = item->prevItem;
	}
    
	return item;    
}

#pragma mark - Runtime Callbacks

void __CFArrayDeallocCallback(CFTypeRef object)
{
    CFArrayRef array = (CFArrayRef)object;
    CFArrayRemoveAllItems(array);
    __CFArrayInit(array);
}

//void __CFArrayEncodeCallback(CFTypeRef object, CFCoderRef coder)
//{
//    CFArrayRef array = (CFArrayRef)object;
//    if (array == NULL)
//    {
//        CFCoderEncodeUInt8(coder, 0);
//        return;
//    }
//    
//    UInt8 arrayCount = CFArrayGetCount(array);
//    CFCoderEncodeUInt8(coder, arrayCount);
//    
//    CFArrayIterator iterator = CFArrayGetIterator(array);    
//    while (iterator!= NULL) 
//    {
//        CFCoderEncodeObject(coder, iterator->object);
//        iterator = CFArrayMoveForward(iterator);
//    }
//}
//
//CFTypeRef __CFArrayDecodeCallback(CFCoderRef coder)
//{    
//    UInt8 arrayCount = CFCoderDecodeUInt8(coder);
//    UInt8 count;
//
//    CFArrayRef array = CFArrayCreateMutable();
//
//    for (count = 0; count < arrayCount; count++) 
//    {
//        CFTypeRef object = CFCoderDecodeObject(coder);
//        CFArrayAddObject(array, object);
//        CFRelease(object);
//    }   
//
//    return array;
//}
