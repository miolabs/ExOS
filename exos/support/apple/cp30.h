#ifndef APPLE_CP30_H
#define APPLE_CP30_H

#include <stdbool.h>

typedef enum
{
	CP30_REG_DEVICE_VERSION = 0,
	CP30_REG_AUTH_REV,
	CP30_REG_AUTH_PROTO_MAJOR_VER,
	CP30_REG_AUTH_PROTO_MINOR_VER,
   	CP30_REG_DEVICE_ID,
   	CP30_REG_ERROR_CODE,

	CP30_REG_AUTH_CONTROL_AND_STATUS = 0x10,
	CP30_REG_CHALLENGE_RESP_DATA_LENGTH,
	CP30_REG_CHALLENGE_RESP_DATA,

	CP30_REG_CHALLENGE_DATA_LENGTH = 0x20,
	CP30_REG_CHALLENGE_DATA,

	CP30_REG_ACCESORY_CERTIFICATE_DATA_LENGTH = 0x30,
	CP30_REG_ACCESORY_CERTIFICATE_DATA1,
	CP30_REG_ACCESORY_CERTIFICATE_DATA2,
	CP30_REG_ACCESORY_CERTIFICATE_DATA3,
	CP30_REG_ACCESORY_CERTIFICATE_DATA4,
	CP30_REG_ACCESORY_CERTIFICATE_DATA5,

	CP30_REG_SELFTEST_STATUS = 0x40,
	CP30_REG_DEVICE_CERTIFICATE_SERIAL_NUMBER = 0x4E,
	CP30_REG_SLEEP = 0x60,
} cp30_reg_t;

typedef enum
{
	CP30_ERR_NO_ERROR = 0,
	CP30_ERR_RSVD,
	CP30_ERR_INVALID_REG,
	CP30_ERR_SEQ_ERROR = 5,
	CP30_ERR_INTERNAL,
} cp30_error_code_t;

// REG_AUTH_CONTROL_AND_STATUS status values
#define CP30_ACAS_ERROR	(1 << 7)
#define CP30_ACAS_RESULT_BIT	4
#define CP30_ACAS_RESULT_MASK	(0x7 << CP30_ACAS_RESULT_BIT)

// REG_AUTH_CONTROL_AND_STATUS write values
#define CP30_ACAS_START (1 << 0)


// prototypes
bool apple_cp30_initialize();
bool apple_cp30_read_device_id(unsigned long *pdevice_id);
bool apple_cp30_read_acc_cert_length(unsigned short *plength);
bool apple_cp30_read_acc_cert(unsigned char *buffer, unsigned short length);
bool apple_cp30_begin_challenge(unsigned char *challenge, unsigned short length, unsigned short *presp_len);
bool apple_cp30_read_challenge_resp(unsigned char *data, unsigned short length);

#endif // APPLE_CP30_H


