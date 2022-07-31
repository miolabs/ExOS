#include "iap_fid.h"
#include "iap_comm.h"
#include "cp20.h"
#include <kernel/machine/hal.h>

#ifndef IAP_ACCESORY_NAME
#define IAP_ACCESORY_NAME "MIO Labs Test Accesory"
#endif

#define IAP_ACCESORY_FW_VERSION_MAJOR 1
#define IAP_ACCESORY_FW_VERSION_MINOR 0
#define IAP_ACCESORY_FW_VERSION_REV 0
#define IAP_ACCESORY_HW_VERSION_MAJOR 1
#define IAP_ACCESORY_HW_VERSION_MINOR 0
#define IAP_ACCESORY_HW_VERSION_REV 0
#ifndef IAP_ACCESORY_MANUFACTURER
#define IAP_ACCESORY_MANUFACTURER "MIO Research Labs"
#endif
#ifndef IAP_ACCESORY_MODEL
#define IAP_ACCESORY_MODEL "uMFi Board 0.1"
#endif
#ifndef IAP_ACCESORY_SERIAL
#define IAP_ACCESORY_SERIAL "00000"
#endif
#define IAP_ACCESORY_RF_CERTIFICATION 0
#ifndef IAP_ACCESORY_BUNDLE_SEED
#define IAP_ACCESORY_BUNDLE_SEED "___exos___"
#endif

#ifndef IAP_ACCESORY_CAPS
#define IAP_ACCESORY_CAPS IAP_ACCESORY_CAPS_TOKENF_APPLICATION
#endif

static const unsigned char _supported_lingos[] = { IAP_LINGO_GENERAL };
static unsigned long _device_id;

static int _fid_callback(IAP_FID_TOKEN *token, IAP_FID_TOKEN_VALUES *values, unsigned char *buffer);

static int _fill_token(unsigned char *ptr, IAP_FID_TOKEN token, unsigned char *data, int data_length)
{
	int offset = 0;
	ptr[offset++] = data_length + 2;
	ptr[offset++] = token >> 8;
	ptr[offset++] = token;
	for(int i = 0; i < data_length; i++) 
		ptr[offset++] = data[i];
	return offset;
}

int iap_fid_fill(unsigned char *payload, int max_payload)
{
	//FIXME: check used buffer against max_payload

	if (!apple_cp2_read_device_id(&_device_id))
		return 0;
	
	unsigned char buffer[32];
	int count = 0;
	int offset = 1;
	IAP_FID_TOKEN token = IAP_FID_TOKEN_IDENTIFY;
	IAP_FID_TOKEN_VALUES values;
	while(_fid_callback(&token, &values, buffer))
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

static int _fid_callback(IAP_FID_TOKEN *token, IAP_FID_TOKEN_VALUES *values, unsigned char *buffer)
{
	static int index = 0;
	int count;
	int offset = 0;
	switch(*token)
	{
		case IAP_FID_TOKEN_IDENTIFY: 
			buffer[offset++] = sizeof(_supported_lingos);
			for(int i = 0; i < sizeof(_supported_lingos); i++)
				buffer[offset++] = _supported_lingos[i];
			_fill32(buffer, &offset, 2);
			_fill32(buffer, &offset, _device_id);
			*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_IDENTIFY, 
				.Data = buffer, .DataLength = offset };
			*token = IAP_FID_TOKEN_ACCESORY_CAPS;
			return 1;
		case IAP_FID_TOKEN_ACCESORY_CAPS:
			_fill32(buffer, &offset, 0); // high bits
			_fill32(buffer, &offset, IAP_ACCESORY_CAPS);
			*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_CAPS, 
				.Data = buffer, .DataLength = offset };
			*token = IAP_FID_TOKEN_ACCESORY_INFO;
			index = 0;
			return 1;
		case IAP_FID_TOKEN_ACCESORY_INFO:
			switch(index)
			{
				case 0:
					buffer[offset++] = IAP_ACCESORY_INFO_NAME;
					offset += __str_copy(buffer + offset, IAP_ACCESORY_NAME, 255) + 1;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 1:
					buffer[offset++] = IAP_ACCESORY_INFO_FW_VERSION;
					buffer[offset++] = IAP_ACCESORY_FW_VERSION_MAJOR;
					buffer[offset++] = IAP_ACCESORY_FW_VERSION_MINOR;
					buffer[offset++] = IAP_ACCESORY_FW_VERSION_REV;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++; 
					return 1;
				case 2:
					buffer[offset++] = IAP_ACCESORY_INFO_HW_VERSION;
					buffer[offset++] = IAP_ACCESORY_HW_VERSION_MAJOR;
					buffer[offset++] = IAP_ACCESORY_HW_VERSION_MINOR;
					buffer[offset++] = IAP_ACCESORY_HW_VERSION_REV;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 3:
					buffer[offset++] = IAP_ACCESORY_INFO_MANUFACTURER;
					offset += __str_copy(buffer + offset, IAP_ACCESORY_MANUFACTURER, 255) + 1;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 4:
					buffer[offset++] = IAP_ACCESORY_INFO_MODEL;
					offset += __str_copy(buffer + offset, IAP_ACCESORY_MODEL, 255) + 1;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 5:
					buffer[offset++] = IAP_ACCESORY_INFO_SERIAL;
					offset += __str_copy(buffer + offset, IAP_ACCESORY_SERIAL, 255) + 1;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 6:
					buffer[offset++] = IAP_ACCESORY_INFO_RF_CERTIFICATION;
					_fill32(buffer, &offset, IAP_ACCESORY_RF_CERTIFICATION);
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index++;
					return 1;
				case 7:
					buffer[offset++] = IAP_ACCESORY_INFO_MAX_INCOMING_PAYLOAD;
					buffer[offset++] = IAP_MAX_PACKET_BUFFER >> 8;
					buffer[offset++] = IAP_MAX_PACKET_BUFFER & 0xFF;
					*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_ACCESORY_INFO, 
						.Data = buffer, .DataLength = offset };
					index = 0;
					*token = IAP_FID_TOKEN_EA_PROTOCOL;
					return 1;
			}
			break;
		case IAP_FID_TOKEN_EA_PROTOCOL:
			buffer[offset++] = ++index;
			APPLE_IAP_PROTOCOL_MANAGER *iap = iap_comm_get_protocol(index);
			offset += __str_copy(buffer + offset, iap->Name, 255) + 1;
			*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_EA_PROTOCOL, 
				.Data = buffer, .DataLength = offset };
			if (index >= iap_comm_get_protocol_count())
			{
				index = 0;
				*token = IAP_FID_TOKEN_BUNDLE_SEED_ID_PREF;
			}
			return 1;
		case IAP_FID_TOKEN_BUNDLE_SEED_ID_PREF:
			offset += __str_copy(buffer + offset, IAP_ACCESORY_BUNDLE_SEED, 255) + 1;
			*values = (IAP_FID_TOKEN_VALUES) { .Token = IAP_FID_TOKEN_BUNDLE_SEED_ID_PREF, 
				.Data = buffer, .DataLength = offset };
			*token = IAP_FID_TOKEN_INVALID;
			return 1;
	}
	index = 0;
	return 0;
}
