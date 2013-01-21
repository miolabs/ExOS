#ifndef HAL_MCI_HAL_H
#define HAL_MCI_HAL_H

#include <misc/sdcard_mci.h>



// prototypes
void hal_mci_initialize();
SD_ERROR hal_mci_send_cmd(unsigned char cmd, unsigned long arg, MCI_WAIT_FLAGS flags);
void hal_mci_set_speed(int high);
int hal_mci_read_resp(unsigned char *buf, int resp_bytes);
unsigned long hal_mci_read_short_resp();
SD_ERROR hal_mci_read_data_blocks(unsigned char *buf, int count, MCI_BLOCK_SIZE size);
SD_ERROR hal_mci_write_data_blocks(unsigned char *buf, int count, MCI_BLOCK_SIZE size);

#endif // HAL_MCI_HAL_H
