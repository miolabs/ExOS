//
//  check_ECFNull.c
//  EmbeddedCoreFoundation
//
//  Created by Abelardo Sanchez Ribera on 05/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#import <EmbeddedCoreFoundation/EmbeddedCoreFoundation.h>
#include <assert.h>
#include "ECFNull.h"
#include "check_ECFNull.h"

void test_ECFNull_Creation()
{
	ECFBaseResetRetainCounter();
	ECFNullRef nul = ECFNullCreate();
	assert(nul != NULL); // "ECFNull not created"
	ECFRelease(nul);
	assert(ECFBaseGetRetainCounter() == 0); // Retain counter must be 0
}

void test_ECFNull_EncodeObject()
{
	ECFBaseResetRetainCounter();
	// Create Objects
	ECFNullRef nul = ECFNullCreate();
	
	// Encode
	ECFCoderRef coder = ECFCoderCreate(NULL);
	ECFCoderEncodeObject(coder, nul);
	
	ECFNullRef coderNul = ECFCoderDecodeObject(coder);
	
	// Check encoded objects
	assert(coderNul == NULL);
	
	// Final Release
	ECFRelease(nul);
	ECFRelease(coderNul);
	ECFRelease(coder);
	int rc = ECFBaseGetRetainCounter();
	assert(rc == 0); // Retain counter must be 0
}

