#include "iap2_fid.h"
#include "cp20.h"
#include <kernel/machine/hal.h>

#define IAP2_ACCESORY_NAME "MIO Accesory Name"
#define IAP2_ACCESORY_FW_VERSION_MAJOR 1
#define IAP2_ACCESORY_FW_VERSION_MINOR 0
#define IAP2_ACCESORY_FW_VERSION_REV 0
#define IAP2_ACCESORY_HW_VERSION_MAJOR 1
#define IAP2_ACCESORY_HW_VERSION_MINOR 0
#define IAP2_ACCESORY_HW_VERSION_REV 0
#define IAP2_ACCESORY_MANUFACTURER "MIO Research Labs"
#define IAP2_ACCESORY_MODEL "uMFi"
#define IAP2_ACCESORY_SERIAL "00000"
#define IAP2_ACCESORY_RF_CERTIFICATION 0
#define IAP2_ACCESORY_EA_PROTOCOL "com.miolabs.iap2test"
#define IAP2_ACCESORY_BUNDLE_SEED "A1234"

#ifndef IAP2_ACCESORY_CAPS
#define IAP2_ACCESORY_CAPS IAP2_ACCESORY_CAPS_TOKENF_APPLICATION
#endif

static int _fid_index = 0;
static const unsigned char _supported_lingos[] = { IAP2_LINGO_GENERAL };
static unsigned long _device_id;

static int _fid_callback(IAP2_FID_TOKEN_VALUES *values, unsigned char *buffer);

static int _fill_token(unsigned char *ptr, IAP2_FID_TOKEN token, unsigned char *data, int data_length)
{
	int offset = 0;
	ptr[offset++] = data_length + 2;
	ptr[offset++] = token >> 8;
	ptr[offset++] = token;
	for(int i = 0; i < data_length; i++) 
		ptr[offset++] = data[i];
	return offset;
}

int iap2_fid_fill(unsigned char *payload, int max_payload)
{
	//FIXME: use max_payload

	if (!apple_cp2_read_device_id(&_device_id))
		return 0;
	
	unsigned char buffer[32];
	int count = 0;
	int offset = 1;
	IAP2_FID_TOKEN_VALUES values;
	while(_fid_callback(&values, buffer))
	{
		int length = _fill_token(payload + offset, values.Token, values.Data, values.DataLength);
		offset += length;
		count++;
	}
	payload[0] = count;
	return offset;
}

static void _fill32(unsigned char *buffer, int *poffset, unsigned long value)
{
	int offset = *poffset;
	buffer[offset++] = value >> 24;
	buffer[offset++] = value >> 16;
	buffer[offset++] = value >> 8;
	buffer[offset++] = value;
	*poffset = offset;
}

static int _fid_callback(IAP2_FID_TOKEN_VALUES *values, unsigned char *buffer)
{
	int offset = 0;
	switch(_fid_index++)
	{
		case 0: 
			buffer[offset++] = sizeof(_supported_lingos);
			for(int i = 0; i < sizeof(_supported_lingos); i++)
				buffer[offset++] = _supported_lingos[i];
			_fill32(buffer, &offset, 2);
			_fill32(buffer, &offset, _device_id);
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_IDENTIFY, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 1:
			_fill32(buffer, &offset, 0); // high bits
			_fill32(buffer, &offset, IAP2_ACCESORY_CAPS);
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_CAPS, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 2:
			buffer[offset++] = IAP2_ACCESORY_INFO_NAME;
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_NAME, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 3:
			buffer[offset++] = IAP2_ACCESORY_INFO_FW_VERSION;
			buffer[offset++] = IAP2_ACCESORY_FW_VERSION_MAJOR;
			buffer[offset++] = IAP2_ACCESORY_FW_VERSION_MINOR;
			buffer[offset++] = IAP2_ACCESORY_FW_VERSION_REV;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 4:
			buffer[offset++] = IAP2_ACCESORY_INFO_HW_VERSION;
			buffer[offset++] = IAP2_ACCESORY_HW_VERSION_MAJOR;
			buffer[offset++] = IAP2_ACCESORY_HW_VERSION_MINOR;
			buffer[offset++] = IAP2_ACCESORY_HW_VERSION_REV;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 5:
			buffer[offset++] = IAP2_ACCESORY_INFO_MANUFACTURER;
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_MANUFACTURER, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 6:
			buffer[offset++] = IAP2_ACCESORY_INFO_MODEL;
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_MODEL, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 7:
			buffer[offset++] = IAP2_ACCESORY_INFO_SERIAL;
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_SERIAL, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 8:
			buffer[offset++] = IAP2_ACCESORY_INFO_RF_CERTIFICATION;
			_fill32(buffer, &offset, IAP2_ACCESORY_RF_CERTIFICATION);
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_ACCESORY_INFO, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 9:
			buffer[offset++] = 1;
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_EA_PROTOCOL, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_EA_PROTOCOL, 
				.Data = buffer, .DataLength = offset };
			return 1;
		case 10:
			offset += __str_copy(buffer + offset, IAP2_ACCESORY_BUNDLE_SEED, 255) + 1;
			*values = (IAP2_FID_TOKEN_VALUES) { .Token = IAP2_FID_TOKEN_BUNDLE_SEED_ID_PREF, 
				.Data = buffer, .DataLength = offset };
			return 1;
	}
	return 0;
}
