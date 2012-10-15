//
//  CFBase.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 24/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#include "CFBase.h"

int mRetainCounter = 0;

/*CFRange __CFRangeMake(UInt16 loc, UInt16 len) 
{
    CFRange range;
    range.location = loc;
    range.length = len;
    return range;
}*/

bool CFBaseIsType(CFTypeRef object, char *type)
{
	if (object == NULL || type == NULL)
		return FALSE;
	
    CFBase *base = (CFBase *)object;
    if (strncmp(base->type, type, kCFCoderTypeLen) == 0)
        return TRUE;
    
    return FALSE;
}


CFTypeRef CFAllocatorCreate(char *type, UInt16 size, CFAllocatorsCallbacks *callbacks)
{
    CFTypeRef object = malloc(size);
    if (object == NULL)
        return NULL;
    
    CFBase *base = (CFBase *)object;
    base->rc = 1;
    base->type = type;
    base->callbacks = callbacks;
    if (base->callbacks != NULL)
        if (base->callbacks->allocCallback != NULL)
            base->callbacks->allocCallback(object);

    mRetainCounter++;

    return object;
}

void CFAllocatorDestroy(CFTypeRef object)
{
    if (object == NULL)
        return;
    
    CFBase *base = (CFBase *)object;
    if (base->callbacks != NULL)
        if (base->callbacks->deallocCallback != NULL)
            base->callbacks->deallocCallback(object);
    
    free(object);
}

void CFRetain(CFTypeRef object)
{
    if (object == NULL)
        return;
    
    CFBase *base = object;    
    base->rc++;  
    
    mRetainCounter++;
}

void CFRelease(CFTypeRef object)
{
    if (object == NULL)
        return;
    
    CFBase *base = object;    
    base->rc--;
    
    /*
     // With Pointer Pool implementation 
     if (base->rc == 0)
     CFRuntimeAddToPointerPool(object);
     */
    
    // Without Pointer Pool implementation
    if (base->rc == 0)
        CFAllocatorDestroy(object);    
    
    mRetainCounter--;
}

int CFBaseGetRetainCounter(void)
{
    return mRetainCounter;
}

void CFBaseResetRetainCounter(void)
{
    mRetainCounter = 0;
}

