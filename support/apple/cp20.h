#ifndef APPLE_CP20_H
#define APPLE_CP20_H

typedef enum
{
	CP20_REG_DEVICE_VERSION = 0,
	CP20_REG_FIRMWARE_VERSION,
	CP20_REG_AUTH_PROTO_MAJOR_VER,
	CP20_REG_AUTH_PROTO_MINOR_VER,
   	CP20_REG_DEVICE_ID,
   	CP20_REG_ERROR_CODE,

	CP20_REG_AUTH_CONTROL_AND_STATUS = 0x10,
   	CP20_REG_SIGNATURE_DATA_LENGTH,
   	CP20_REG_SIGNATURE_DATA,

	CP20_REG_CHALLENGE_DATA_LENGTH = 0x20,
	CP20_REG_CHALLENGE_DATA,

	CP20_REG_ACCESORY_CERTIFICATE_DATA_LENGTH = 0x30,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE1,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE2,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE3,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE4,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE5,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE6,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE7,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE8,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE9,
	CP20_REG_ACCESORY_CERTIFICATE_DATA_PAGE10,

	CP20_REG_SELFTEST_CONTROL_AND_STATUS = 0x40,
	CP20_REG_SEC = 0x4D,

	CP20_REG_DEVICE_CERTIFICATE_DATA_LENGTH = 0x50,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE1,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE2,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE3,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE4,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE5,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE6,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE7,
	CP20_REG_DEVICE_CERTIFICATE_DATA_PAGE8,
} CP20_REG;

typedef enum
{
	CP20_PROC_NOP = 0,
	CP20_PROC_START_SIGNATURE_GENERATION,
	CP20_PROC_START_CHALLENGE_GENERATION,
	CP20_PROC_START_SIGNATURE_VERIFICATION,
	CP20_PROC_START_CHALLENGE_VERIFICATION,
} CP20_PROC_CONTROL;

// prototypes
int apple_cp20_initialize();
int apple_cp2_read_device_id(unsigned long *pdevice_id);
int apple_cp2_read_acc_cert_length(unsigned short *plength);
int apple_cp2_read_acc_cert_page(int page, unsigned char *buffer, int length);
int apple_cp2_get_auth_signature(unsigned char *challenge, int length, unsigned char *sig);

#endif // APPLE_CP20_H


