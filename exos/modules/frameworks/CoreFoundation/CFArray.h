//
//  CFArray.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 18/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFArray_h
#define EmbeddedCoreFoundation_CFArray_h

#include "CFBase.h"


typedef struct __CFArrayItem
{
    struct __CFArrayItem *nextItem;
    struct __CFArrayItem *prevItem;
    CFTypeRef object;
    
}__CFArrayItem;

typedef __CFArrayItem *CFArrayIterator;

typedef struct CFArray
{
    CFBase _base;
    __CFArrayItem *startItem;
    __CFArrayItem *endItem;
    int count;
    
}CFArray;

typedef CFArray * CFArrayRef;

CFArrayRef CFArrayCreateMutable(void);

int CFArrayGetCount(CFArrayRef array);

void CFArrayAddObject(CFArrayRef array, CFTypeRef object);

void CFArrayRemoveObject(CFArrayRef array, CFTypeRef object);
void CFArrayRemoveObjectAtIndex(CFArrayRef array, unsigned int index);
void CFArrayRemoveAllItems(CFArrayRef array);

CFTypeRef CFArrayGetObjectAtIndex(CFArrayRef array, int index);

// Iterators

CFArrayIterator CFArrayGetIterator(CFArrayRef array);
void CFArrayAddIterator(CFArrayRef array, CFArrayIterator iterator);
CFArrayIterator CFArrayRemoveIterator(CFArrayRef array, CFArrayIterator iterator);
CFArrayIterator CFArrayMoveForward(CFArrayIterator iterator);
CFArrayIterator CFArrayMoveBackwards(CFArrayIterator iterator);

// Private

void __CFArrayRegisterObject(void);

#endif
