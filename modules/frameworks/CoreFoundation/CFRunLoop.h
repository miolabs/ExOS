//
//  CFRunLoop.h
//  EmbeddedCoreFoundation
//
//  Created by Javier Segura Perez on 12/07/11.
//  Copyright 2011 MIO Labs. All rights reserved.
//

#ifndef EmbeddedCoreFoundation_CFRunLoop_h
#define EmbeddedCoreFoundation_CFRunLoop_h

#include "CFBase.h"
#include "CFString.h"


// Timers
typedef struct CFRunLoopTimer
{
    CFBase _base;
    UInt32 numberOfTicks;
    UInt32 tickCount;
    DWORD  lastTick;
    bool active;
    void *callback;
    CFTypeRef source;
    void *info;    

}CFRunLoopTimer;

typedef CFRunLoopTimer * CFRunLoopTimerRef;
typedef void (*CFRunLoopTimerCallBack)(CFRunLoopTimerRef timer, void *info);

CFRunLoopTimerRef CFRunLoopTimerCreate(double interval, CFRunLoopTimerCallBack callout);

void CFRunLoopAddTimer(CFRunLoopTimerRef timer);
void CFRunLoopRemoveTimer(CFRunLoopTimerRef timer);

void CFRunLoopTimerValidate(CFRunLoopTimerRef timer);
void CFRunLoopTimerInvalidate(CFRunLoopTimerRef timer);

// Sources
typedef void (*CFRunLoopSourceCallback)(void *source);
typedef struct CFRunLoopSource
{
    CFBase _base;
    bool hasToRemove;
    void *info;
    CFRunLoopSourceCallback callback;    
    
}CFRunLoopSource;

typedef CFRunLoopSource * CFRunLoopSourceRef;

// RunLoops
CFRunLoopSourceRef CFRunLoopCreateRunLoopSource(CFRunLoopSourceCallback srcCallback, void *info);
void CFRunLoopAddSource(CFRunLoopSourceRef source);
void CFRunLoopRemoveSource(CFRunLoopSourceRef source);
void CFRunLoopRun(void);
void CFRunLoopHalt(void);

#endif
