//
//  main.c
//  CoreFoundation
//
//  Created by GodShadow on 10/12/12.
//  Copyright (c) 2012 MIOLabs. All rights reserved.
//

#include <stdio.h>

#include "check_CFDictionary.h"
#include "check_CFNumber.h"
#include "check_CFUUID.h"

int main (int argc, const char * argv[])
{
	test_CFDictionary_Creation();
	test_CFDictionary_SetObject();
	test_CFDictionary_RemoveObject();
	test_CFDictionary_EncodeObject();
	test_CFDictionary_AddEntriesFromDictionary();
	
	test_CFNumber_Creation();
	test_CFNumber_SetValue();
	test_CFNumber_EncodeObject();
	
	test_CFUUID_Creation();
	test_CFUUID_SetValue();
	test_CFUUID_EncodeObject();
		
    return 0;
}
