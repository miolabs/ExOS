//
//  check_CFDictionary.c
//  EmbeddedCoreFoundation
//
//  Created by Abelardo Sanchez Ribera on 02/12/11.
//  Copyright (c) 2011 MIO Labs. All rights reserved.
//

#include <assert.h>
#include "CFDictionary.h"
#include "check_CFDictionary.h"

void test_CFDictionary_Creation()
{
	CFBaseResetRetainCounter();
	CFDictionaryRef dictionary = CFDictionaryCreateMutable();
	assert(dictionary != NULL); // "dictionary not created"
	CFRelease(dictionary);
	assert(dictionary->_base.rc == 0); // "dictionary not freed correctly"
	assert(CFBaseGetRetainCounter() == 0); // Retain counter must be 0
}

void test_CFDictionary_SetObject()
{
	CFBaseResetRetainCounter();
	// Create Objects
	CFDictionaryRef dictionary = CFDictionaryCreateMutable();
	CFStringRef string = CFSTR("this is a string");
	UInt8 byte = 0xAA;
	CFDataRef data = CFDataCreateMutableWithBytes(&byte, 1);
	CFArrayRef array = CFArrayCreateMutable();
	CFArrayAddObject(array, string);
	
	// Set Objects into dictionary
	CFDictionarySetObjectForKey(dictionary, string, CFSTR("string key"));
	assert(CFDictionaryGetCount(dictionary) == 1); // "Dictionary must have 1 object inside"
	
	CFDictionarySetObjectForKey(dictionary, data, CFSTR("data key"));
	assert(CFDictionaryGetCount(dictionary) == 2); // "Dictionary must have 1 object inside"
	
	CFDictionarySetObjectForKey(dictionary, array, CFSTR("array key"));
	assert(CFDictionaryGetCount(dictionary) == 3); // "Dictionary must have 1 object inside"
	
	// Check Objects
	CFStringRef dictionaryString = CFDictionaryGetObjectForKey(dictionary, CFSTR("string key"));
	assert(CFStringIsEqualToString(dictionaryString, string)); // "Strings aren't equal"
	CFRelease(string);
	
	CFDataRef dictionaryData = CFDictionaryGetObjectForKey(dictionary, CFSTR("data key"));
	assert(dictionaryData == data); // "Datas aren't equal"
	CFRelease(data);
	
	CFArrayRef dictionaryArray = CFDictionaryGetObjectForKey(dictionary, CFSTR("array key"));
	assert(dictionaryArray == array); // "Arrays aren't equal"
	CFRelease(array);
	
	// Final Release
	CFRelease(dictionary);
	assert(dictionary->_base.rc == 0); // "dictionary not freed correctly"
	assert(CFBaseGetRetainCounter() == 6); // Retain counter must be 6 (because the constatnt strings)
}

void test_CFDictionary_RemoveObject()
{
	CFBaseResetRetainCounter();
	// Create Objects
	CFDictionaryRef dictionary = CFDictionaryCreateMutable();
	CFStringRef string = CFSTR("this is a string");
	UInt8 byte = 0xAA;
	CFDataRef data = CFDataCreateMutableWithBytes(&byte, 1);
	CFArrayRef array = CFArrayCreateMutable();
	CFStringRef stringInArray = CFSTR("string inside array");
	CFArrayAddObject(array, stringInArray);
	CFRelease(stringInArray);
	
	// Set Objects into dictionary
	CFDictionarySetObjectForKey(dictionary, string, CFSTR("string key"));
	assert(CFDictionaryGetCount(dictionary) == 1); // "Dictionary must have 1 object inside"
	CFRelease(string);
	
	CFDictionarySetObjectForKey(dictionary, data, CFSTR("data key"));
	assert(CFDictionaryGetCount(dictionary) == 2); // "Dictionary must have 1 object inside"
	CFRelease(data);
	
	CFDictionarySetObjectForKey(dictionary, array, CFSTR("array key"));
	assert(CFDictionaryGetCount(dictionary) == 3); // "Dictionary must have 1 object inside"
	CFRelease(array);
	
	// Remove Objects
	CFDictionaryRemoveObjectForKey(dictionary, CFSTR("string key"));
	assert(CFDictionaryGetCount(dictionary) == 2); // "Dictionary must have 1 object inside"
	
	CFDictionaryRemoveObjectForKey(dictionary, CFSTR("data key"));
	assert(CFDictionaryGetCount(dictionary) == 1); // "Dictionary must have 1 object inside"
	
	CFDictionaryRemoveObjectForKey(dictionary, CFSTR("array key"));
	assert(CFDictionaryGetCount(dictionary) == 0); // "Dictionary must have 1 object inside"
	
	// Final Release
	CFRelease(dictionary);
	assert(dictionary->_base.rc == 0); // "dictionary not freed correctly"
	//Check that Objects are released
//	assert(array->_base.rc == 0); // "Array not released properly"
//	assert(data->_base.rc == 0); // "Data not released properly"
//	assert(string->_base.rc == 0); // "String not released properly"
	int rc = CFBaseGetRetainCounter();
	assert(rc == 6); // Retain counter must be 6
}

void test_CFDictionary_EncodeObject()
{
	CFBaseResetRetainCounter();
	// Create Objects
	CFDictionaryRef dictionary = CFDictionaryCreateMutable();
	CFStringRef string = CFSTR("this is a string");
	UInt8 byte = 0xAA;
	CFDataRef data = CFDataCreateMutableWithBytes(&byte, 1);
	CFArrayRef array = CFArrayCreateMutable();
	CFStringRef stringInArray = CFSTR("string inside array");
	CFArrayAddObject(array, stringInArray);
	CFRelease(stringInArray);
	
	// Set Objects into dictionary
	CFDictionarySetObjectForKey(dictionary, string, CFSTR("string key"));
	assert(CFDictionaryGetCount(dictionary) == 1); // "Dictionary must have 1 object inside"
	CFRelease(string);
	
	CFDictionarySetObjectForKey(dictionary, data, CFSTR("data key"));
	assert(CFDictionaryGetCount(dictionary) == 2); // "Dictionary must have 1 object inside"
	CFRelease(data);
	
	CFDictionarySetObjectForKey(dictionary, array, CFSTR("array key"));
	assert(CFDictionaryGetCount(dictionary) == 3); // "Dictionary must have 1 object inside"
	CFRelease(array);
	
	// Encode Dictionary
//	CFCoderRef coder = CFCoderCreate(NULL);
//	CFCoderEncodeObject(coder, dictionary);
//	
//	CFDictionaryRef coderDictionary = CFCoderDecodeObject(coder);
	
	// Check encoded objects
//	CFStringRef coderString = CFDictionaryGetObjectForKey(coderDictionary, CFSTR("string key"));
//	assert(CFStringIsEqualToString(string, coderString)); // Strings must be equal
//	
//	CFDataRef coderData = CFDictionaryGetObjectForKey(coderDictionary, CFSTR("data key"));
//	UInt8 coderByte = 0;
//	CFDataGetByteAtIndex(coderData, &coderByte, 0);
//	assert(byte == coderByte); // Bytes must be equal
//	
//	CFArrayRef coderArray = CFDictionaryGetObjectForKey(coderDictionary, CFSTR("array key"));
//	CFStringRef coderStringInArray = CFArrayGetObjectAtIndex(coderArray, 0);
//	assert(CFStringIsEqualToString(stringInArray, coderStringInArray)); // Array Items must be equal
	
	
	// Final Release
	CFRelease(dictionary);
//	CFRelease(coderDictionary);
//	CFRelease(coder);
	int rc = CFBaseGetRetainCounter();
	assert(rc == 6); // Retain counter must be 6
}

void test_CFDictionary_AddEntriesFromDictionary()
{
	CFBaseResetRetainCounter();
	
	//
	// Create Objects
	//
	
	// keys
	CFStringRef stringKey = CFSTR("string key");
	CFStringRef dataKey = CFSTR("data key");
	CFStringRef arrayKey = CFSTR("array key");
	
	// dictionaries
	CFDictionaryRef dictionary1 = CFDictionaryCreateMutable();
	CFDictionaryRef dictionary2 = CFDictionaryCreateMutable();
	
	// objects
	CFStringRef string1 = CFSTR("this is a string");
	CFStringRef string2 = CFSTR("this is other string");
	
	UInt8 byte = 0xAA;
	CFDataRef data = CFDataCreateMutableWithBytes(&byte, 1);
	
	CFArrayRef array = CFArrayCreateMutable();
	CFStringRef stringInArray = CFSTR("string inside array");
	CFArrayAddObject(array, stringInArray);
	
	//
	// Set Objects into dictionaries
	//
	
	// Dictionary 1 (original, contains 1 string and 1 data)
	CFDictionarySetObjectForKey(dictionary1, string1, stringKey);
	CFRelease(string1);
	
	CFDictionarySetObjectForKey(dictionary1, data, dataKey);
	CFRelease(data);
	
	// Dictionary 2 (new entries, contains 1 string (same stringKey) and 1 array)
	CFDictionarySetObjectForKey(dictionary2, string2, stringKey); // Different object with the same key
	CFRelease(string2);
	
	CFDictionarySetObjectForKey(dictionary2, array, arrayKey); // New object
	CFRelease(array);
	
	//
	// Add Entries
	//
	
	CFDictionaryAddEntriesFromDictionary(dictionary1, dictionary2);
	
	//
	// Check
	//
	
	assert(CFDictionaryGetCount(dictionary1) == 3); // Dict1 must have 3 objects now
	
	CFStringRef finalString = CFDictionaryGetObjectForKey(dictionary1, stringKey);
	assert(finalString != NULL); // Final String must exist in dictionary 1
	assert(CFStringIsEqualToString(finalString, string2)); // Final String must be string2
	
	CFDataRef finalData = CFDictionaryGetObjectForKey(dictionary1, dataKey);
	assert(finalData != NULL); // Data must exist
	UInt8 finalByte = 0;
	CFDataGetByteAtIndex(finalData, &finalByte, 0);
	assert(finalByte == byte); // Data content must be the same
	
	CFArrayRef finalArray = CFDictionaryGetObjectForKey(dictionary1, arrayKey);
	assert(finalArray != NULL); // Final array must exist in dictionary 1
	assert(CFArrayGetCount(finalArray) == 1); // Final array must contain 1 object
	CFStringRef finalArrayString = CFArrayGetObjectAtIndex(finalArray, 0);
	assert(CFStringIsEqualToString(stringInArray, finalArrayString)); // Final string in array must be the same as StringInArray
	
	
	// Final Release
	CFRelease(stringKey);
	CFRelease(dataKey);
	CFRelease(arrayKey);
	CFRelease(stringInArray);
	CFRelease(dictionary1);
	CFRelease(dictionary2);
	int rc = CFBaseGetRetainCounter();
	assert(rc == 0); // Retain counter must be 0
}
