#ifndef SDCARD_H
#define SDCARD_H

#define SD_NULL ((void *)0)

typedef struct
{
	unsigned long BlockSize;
	unsigned long Blocks;
	unsigned char CID[16];
	unsigned char CSD[16];
	unsigned long OCR;
} SD_INFO;

#define SD_OCR_HCS (1 << 30)

typedef struct __attribute((packed))
{
	unsigned NU:1;
	unsigned CRC:7;

	unsigned :2;
	unsigned FILE_FORMAT:2;
	unsigned TMP_WRITE_PROTECT:1;
	unsigned PERM_WRITE_PROTECT:1;
	unsigned COPY:1;
	unsigned FILE_FORMAT_GRP:1;

	unsigned :5;
	unsigned WRITE_BLK_PARTIAL:1;
	unsigned WRITE_BLK_LEN:4;
	unsigned R2W_FACTOR:3;
	unsigned :2;
	unsigned WP_GRP_ENABLE:1;

	unsigned WP_GRP_SIZE:7;
	unsigned SECTOR_SIZE:7;
	unsigned ERASE_BLK_EN:1;

	unsigned C_SIZEF:27;
	unsigned :2;

	unsigned DS_IMP:1;
	unsigned READ_BL_MISALIGN:1;
	unsigned WRITE_BL_MISALIGN:1;
	unsigned READ_BL_PARTIAL:1;
	
	unsigned READ_BL_LEN:4;
	unsigned CCC:12;
	
	unsigned TRAN_SPEED:8;
	unsigned NSAC:8;
	unsigned TAAC:8;
	
	unsigned :6;
	unsigned CSD_STRUCTURE:2;
} SD_CSD_FLIPPED;

typedef enum
{
	SD_OK_IDLE = 1,
	SD_OK = 0,
    SD_ERROR_TIMEOUT = -1,
    SD_ERROR_TOKEN_TIMEOUT = -2,
    SD_ERROR_BAD_TOKEN = -3,
    SD_ERROR_NOT_IDLE = -4,
    SD_ERROR_BAD_CRC = -5,
	SD_ERROR_BAD_CMD = -6,
	SD_ERROR_WRITE_ERROR = -7,
	SD_ERROR_BAD_STATE = -8,
	SD_ERROR_OVERFLOW = -9,
	SD_ERROR_NOT_READY = -10,
	SD_ERROR_UNSUPPORTED = -11,
} SD_ERROR;

typedef enum
{
	SD_CARD_IDLE = 0,
	SD_CARD_READY,
	SD_CARD_IDENTIFICATION,
	// data transfer modes
	SD_CARD_STAND_BY,
	SD_CARD_TRANSFER,
	SD_CARD_SENDING_DATA,
	SD_CARD_RECEIVE_DATA,
	SD_CARD_PROGRAMMING,
	SD_CARD_DISCONNECT,
} SD_CARD_STATE;

#define SD_CARD_STATUS_STATE_BIT 9
typedef enum
{
	SD_CARD_STATUS_APP_CMD = (1<<5),
	SD_CARD_STATUS_READY_FOR_DATA = (1<<8),
	SD_CARD_STATUS_STATE_MASK = (15<<SD_CARD_STATUS_STATE_BIT),
} SD_CARD_STATUS;

typedef enum
{
	SD_MANU_UNKNOWN = 0,
	SD_MANU_PANASONIC = 1,
	SD_MANU_TOSHIBA = 2,
	SD_MANU_SANDISK = 3,
} SD_MANUFACTURER;

#define SD_OCR_NOT_BUSY	0x80000000

// prototypes
int sd_initialize();
void sd_get_info(SD_INFO *info);
SD_ERROR sd_read_blocks(unsigned long block, unsigned long count, unsigned char *buf);
SD_ERROR sd_write_blocks(unsigned long block, unsigned long count, unsigned char *buf);
SD_ERROR sd_get_card_state(SD_CARD_STATE *pstate);

// hal functions
void sd_hw_initialize();
int sd_hw_card_reset();
int sd_hw_card_identification(void *cid, unsigned short *prca);
int sd_hw_select_card();
SD_ERROR sd_send_cmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length);
SD_ERROR sd_send_acmd_resp(unsigned char cmd, unsigned long arg, void *resp, int resp_length);
SD_ERROR sd_send_op_cond(unsigned long arg, unsigned long *ocr_ptr);

SD_ERROR sd_hw_read_single_block(unsigned long addr, unsigned char *buf);
SD_ERROR sd_hw_read_blocks(unsigned long addr, unsigned long count, unsigned char *buf);
SD_ERROR sd_hw_write_single_block(unsigned long addr, unsigned char *buf);
SD_ERROR sd_hw_write_blocks(unsigned long addr, unsigned long count, unsigned char *buf);
SD_ERROR sd_hw_check_status(SD_CARD_STATE *pstate);

int sd_add_device();

#endif //SDCARD_H
