//
//  check_CFUUID.c
//  EmbeddedCoreFoundation
//
//  Created by Abelardo Sanchez Ribera on 05/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#import "CoreFoundation.h"
#include <assert.h>
#include "CFUUID.h"
#include "check_CFUUID.h"

void test_CFUUID_Creation()
{
	CFBaseResetRetainCounter();
	CFUUIDRef uuid = CFUUIDCreate();
	assert(uuid != NULL); // "uuid not created"
	CFRelease(uuid);
	assert(uuid->_base.rc == 0); // "uuid not freed correctly"
	assert(CFBaseGetRetainCounter() == 0); // Retain counter must be 0
}


void test_CFUUID_SetValue()
{
	CFBaseResetRetainCounter();
	// Create Objects
	UInt8 byte = 0xAA;
	CFUUIDRef uuid = CFUUIDCreateWithByte(byte);
	assert(uuid->uuid[0] == byte
		   && uuid->uuid[1] == byte
		   && uuid->uuid[2] == byte
		   && uuid->uuid[3] == byte
		   && uuid->uuid[4] == byte
		   && uuid->uuid[5] == byte
		   && uuid->uuid[6] == byte
		   && uuid->uuid[7] == byte
		   && uuid->uuid[8] == byte
		   && uuid->uuid[9] == byte
		   && uuid->uuid[10] == byte
		   && uuid->uuid[11] == byte
		   && uuid->uuid[12] == byte
		   && uuid->uuid[13] == byte
		   && uuid->uuid[14] == byte
		   && uuid->uuid[15] == byte);
	
	UInt8 bytesArray[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	CFUUIDRef uuid2 = CFUUIDCreateWithBytes(bytesArray);
	assert(uuid2->uuid[0] == bytesArray[0]
		   && uuid2->uuid[1] == bytesArray[1]
		   && uuid2->uuid[2] == bytesArray[2]
		   && uuid2->uuid[3] == bytesArray[3]
		   && uuid2->uuid[4] == bytesArray[4]
		   && uuid2->uuid[5] == bytesArray[5]
		   && uuid2->uuid[6] == bytesArray[6]
		   && uuid2->uuid[7] == bytesArray[7]
		   && uuid2->uuid[8] == bytesArray[8]
		   && uuid2->uuid[9] == bytesArray[9]
		   && uuid2->uuid[10] == bytesArray[10]
		   && uuid2->uuid[11] == bytesArray[11]
		   && uuid2->uuid[12] == bytesArray[12]
		   && uuid2->uuid[13] == bytesArray[13]
		   && uuid2->uuid[14] == bytesArray[14]
		   && uuid2->uuid[15] == bytesArray[15]);
	
	// Final Release
	CFRelease(uuid);
	CFRelease(uuid2);
	assert(CFBaseGetRetainCounter() == 0); // Retain counter must be 0
}

void test_CFUUID_EncodeObject()
{
	CFBaseResetRetainCounter();
	// Create Objects
	UInt8 byte = 0xAA;
	CFUUIDRef uuid = CFUUIDCreateWithByte(byte);
	UInt8 bytesArray[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	CFUUIDRef uuid2 = CFUUIDCreateWithBytes(bytesArray);
	
	// Encode UUIDs
//	CFCoderRef coder = CFCoderCreate(NULL);
//	CFCoderEncodeObject(coder, uuid);
//	CFCoderEncodeObject(coder, uuid2);
//	
//	CFUUIDRef coderUuid = CFCoderDecodeObject(coder);
//	CFUUIDRef coderUuid2 = CFCoderDecodeObject(coder);
	
	
	// Check encoded objects
//	assert(CFUUIDIsEqualToUUID(uuid, coderUuid)); // first uuid
//	assert(CFUUIDIsEqualToUUID(uuid2, coderUuid2)); // second uuid
	
	
	// Final Release
	CFRelease(uuid);
	CFRelease(uuid2);
//	CFRelease(coderUuid);
//	CFRelease(coderUuid2);
//	CFRelease(coder);
	int rc = CFBaseGetRetainCounter();
	assert(rc == 0); // Retain counter must be 0
}
