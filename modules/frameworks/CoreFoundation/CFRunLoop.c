//
//  CFRunLoop.c
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 12/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#include "CFRunLoop.h"
#include "CFArray.h"

#pragma Platform Specifics Functions

bool mRunLoopRun = TRUE;

// Timers
void __CFRunLoopTimerDeallocCallback(CFTypeRef object);
void __CFRunLoopTimerSourceCallback(void *source);

CFAllocatorsCallbacks mCFRunLoopTimerDefaultAllocators = { NULL, __CFRunLoopTimerDeallocCallback};

#pragma mark - Timers

CFRunLoopTimerRef CFRunLoopTimerCreate(double interval, CFRunLoopTimerCallBack callout)
{
    CFRunLoopTimerRef timer = (CFRunLoopTimerRef)CFAllocatorCreate(kCFRunLoopTimerType, sizeof(CFRunLoopTimer), &mCFRunLoopTimerDefaultAllocators);    
    if (timer != NULL)
    {
        timer->numberOfTicks = (UInt32)(interval * 1000.0);
        timer->tickCount = 0; // between miliseconds
        timer->lastTick = 0;
        timer->active = FALSE;
        timer->callback = callout;
        timer->source = NULL;
    }

    return timer;
}

void CFRunLoopTimerValidate(CFRunLoopTimerRef timer)
{
	if (timer != NULL)
    {
		timer->active = TRUE;
        timer->tickCount = 0;
        timer->lastTick = 0;        
    }
}

void CFRunLoopTimerInvalidate(CFRunLoopTimerRef timer)
{
	if (timer != NULL)
		timer->active = FALSE;
}

void CFRunLoopAddTimer(CFRunLoopTimerRef timer)
{
	if (!timer)
		return;
    CFRunLoopSourceRef timerSource = CFRunLoopCreateRunLoopSource(__CFRunLoopTimerSourceCallback, timer);
    CFRunLoopAddSource(timerSource);
    timer->source = timerSource; // weak reference so we do not use retain
	CFRetain(timerSource);
    CFRunLoopTimerValidate(timer);
    CFRelease(timerSource);
}

void CFRunLoopRemoveTimer(CFRunLoopTimerRef timer)
{
	if (!timer)
		return;
    CFRunLoopSourceRef timerSource = (CFRunLoopSourceRef)timer->source;
    CFRunLoopTimerInvalidate(timer);
    if (timerSource != NULL)
	{
        CFRunLoopRemoveSource(timerSource);
		CFRelease(timerSource);
		timer->source = NULL;
	}
}

void __CFRunLoopTimerSourceCallback(void *source)
{
    if (!source)
        return;
    CFRunLoopSourceRef timerSource = (CFRunLoopSourceRef)source;
    CFRunLoopTimerRef timer = (CFRunLoopTimerRef)timerSource->info;
    
    if (!timer || timer->active == FALSE)
        return;
    
    // Go on with the timer
    if (timer->tickCount >= timer->numberOfTicks)
    {
        timer->tickCount = 0;
        if (timer->callback != NULL)
        {
            CFRunLoopTimerCallBack timerCallback = (CFRunLoopTimerCallBack)timer->callback;
            timerCallback(timer, timer->info);
        }
    }
#ifndef __APPLE__
    // Check if we had 1 ms
    if (CF_platform_runloop_get_tick_count() - timer->lastTick > CF_platform_runloop_get_milisecond_ticks())
    {
        // Update anti-ping flood timer        
        timer->tickCount++;
        timer->lastTick = CF_platform_runloop_get_tick_count();
    }
    else
        timer->tickCount++;
#else
    timer->tickCount++;
#endif
    }

void __CFRunLoopTimerDeallocCallback(CFTypeRef object)
{
    CFRunLoopTimerRef timer = (CFRunLoopTimerRef)object;
	if (timer && timer->source != NULL)
	{
		CFRunLoopSourceRef source = (CFRunLoopSourceRef)timer->source;
		source->hasToRemove = TRUE;
		CFRelease(source);
	}
}

#pragma mark - Run Loop

// Sources
CFAllocatorsCallbacks mCFRunLoopSourceDefaultAllocators = { NULL, NULL};
CFArrayRef mRunLoopSources = NULL;

CFRunLoopSourceRef CFRunLoopCreateRunLoopSource(CFRunLoopSourceCallback srcCallback, void *info)
{
    CFRunLoopSourceRef source = (CFRunLoopSourceRef)CFAllocatorCreate(kCFRunLoopSourceType, sizeof(CFRunLoopSource), NULL);    
    if (source != NULL)
    {
        source->callback = srcCallback;
        source->info = info;
        source->hasToRemove = FALSE;
    }
    
    return source;
}

void CFRunLoopAddSource(CFRunLoopSourceRef source)
{
    if (mRunLoopSources == NULL)
        mRunLoopSources = CFArrayCreateMutable();
    
    if (!source || source->callback == NULL)
        return;
    
    CFArrayAddObject(mRunLoopSources, source);
}

void CFRunLoopRemoveSource(CFRunLoopSourceRef delSource)
{
   if (!delSource || mRunLoopSources == NULL)
        return;
    
    delSource->hasToRemove = TRUE;
}

void CFRunLoopRun(void)
{
   if (mRunLoopSources == NULL)
        mRunLoopSources = CFArrayCreateMutable();

    while (mRunLoopRun) 
    {
        // Checks Sources
        if (CFArrayGetCount(mRunLoopSources) > 0)
        {
            CFArrayIterator iterator = NULL;
            iterator = CFArrayGetIterator(mRunLoopSources);
            while (iterator != NULL)
            {                    
                CFRunLoopSourceRef source = (CFRunLoopSourceRef)iterator->object;
                if (!source->hasToRemove)
                {
                    source->callback(source);
                    iterator = CFArrayMoveForward(iterator);
                }
                else // We have to remove that source     
                {
                    CFRelease(source);
                    iterator = CFArrayRemoveIterator(mRunLoopSources, iterator);
                }
            }
        }
        // Uncoment the line below if you have Pointer Pool Implementation
        //CFRuntimeCheckPointerPool();

#ifdef __APPLE__
        usleep(1000);
#endif
    }
    CFRelease(mRunLoopSources);
}

#pragma mark - Extras

void CFRunLoopHalt(void)
{
    mRunLoopRun = FALSE;
}


