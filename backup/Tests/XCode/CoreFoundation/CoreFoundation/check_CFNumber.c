//
//  check_CFNumber.c
//  EmbeddedCoreFoundation
//
//  Created by Abelardo Sanchez Ribera on 05/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#import <CoreFoundation/CoreFoundation.h>
#include <assert.h>
#include "CFNumber.h"
#include "check_CFNumber.h"

void test_CFNumber_Creation()
{
	CFBaseResetRetainCounter();
	CFNumberRef number = CFNumberCreate();
	assert(number != NULL); // "number not created"
	CFRelease(number);
	assert(number->_base.rc == 0); // "number not freed correctly"
	assert(CFBaseGetRetainCounter() == 0); // Retain counter must be 0
}


void test_CFNumber_SetValue()
{
	CFBaseResetRetainCounter();
	// Create Objects
	UInt8 aUint8 = 0xAA;
	UInt16 aUint16 = 0xAABB;
	CFNumberRef number8 = CFNumberCreate();
	CFNumberRef number16 = CFNumberCreate();
	
	// Set values
	CFNumberSetUInt8(number8, aUint8);
	CFNumberSetUInt16(number16, aUint16);
	
	// get values
	UInt8 newUint8 = CFNumberGetUInt8(number8);
	UInt16 newUint16 = CFNumberGetUInt16(number16);
	
	// check
	assert(aUint8 == newUint8); // UInt8's must be equal
	assert(aUint16 == newUint16); // UInt16's must be equal
	
	// Final Release
	CFRelease(number8);
	CFRelease(number16);
	assert(CFBaseGetRetainCounter() == 0); // Retain counter must be 0
}

void test_CFNumber_EncodeObject()
{
	CFBaseResetRetainCounter();
	// Create Objects
	UInt8 aUint8 = 0xAA;
	UInt16 aUint16 = 0xAABB;
	CFNumberRef number8 = CFNumberCreate();
	CFNumberRef number16 = CFNumberCreate();
	
	// Set values
	CFNumberSetUInt8(number8, aUint8);
	CFNumberSetUInt16(number16, aUint16);
	
	// Encode Numbers
//	CFCoderRef coder = CFCoderCreate(NULL);
//	CFCoderEncodeObject(coder, number8);
//	CFCoderEncodeObject(coder, number16);
//	
//	CFNumberRef coderNumber8 = CFCoderDecodeObject(coder);
//	CFNumberRef coderNumber16 = CFCoderDecodeObject(coder);
	
	// Check encoded objects
//	UInt8 coderUint8 = CFNumberGetUInt8(coderNumber8);
//	assert(coderUint8 == aUint8); // Numbers must be equal
//	
//	UInt16 coderUint16 = CFNumberGetUInt16(coderNumber16);
//	assert(coderUint16 == aUint16); // Numbers must be equal
//		
//	// Final Release
//	CFRelease(number8);
//	CFRelease(number16);
//	CFRelease(coderNumber8);
//	CFRelease(coderNumber16);
//	CFRelease(coder);
	int rc = CFBaseGetRetainCounter();
	assert(rc == 0); // Retain counter must be 0
}
