#ifndef IAP_H
#define IAP_H

#include <support/misc/flash.h>

typedef enum
{
	IAP_SUCCESS  = 0,
	IAP_INVALID_COMMAND = 1,
	IAP_SRC_ADDR_ERROR = 2,
	IAP_DST_ADDR_ERROR = 3,
	IAP_SRC_ADDR_NOT_MAPPED = 4,
	IAP_DST_ADDR_NOT_MAPPED = 5,
	IAP_COUNT_ERROR = 6,
	IAP_INVALID_SECTOR = 7,
	IAP_SECTOR_NOT_BLANK = 8,
	IAP_SECTOR_NOT_PREPARED_FOR_WRITE_OP = 9,
	IAP_COMPARE_ERROR = 10,
	IAP_BUSY = 11,
} IAP_RC;

typedef enum
{
	IAP_PREPARE_SECTOR = 50,
	IAP_COPY_RAM_TO_FLASH = 51,
	IAP_ERASE_SECTOR = 52,
	IAP_BLANK_CHECK_SECTOR = 53,
	IAP_READ_PART_ID = 54,
	IAP_READ_VERSION = 55,
	IAP_COMPARE = 56,
	IAP_REINVOKE_ISP = 57,
	IAP_READ_UID = 58,
} IAP_CMD;

typedef void (*IAP)(unsigned int[], unsigned int[]);

extern const IAP __iap;

// prototypes
void iap_get_flash_info(FLASH_INFO *fi);
int iap_write_block(FLASH_INFO *info, void *addr, const void *data);


#endif // IAP_H

