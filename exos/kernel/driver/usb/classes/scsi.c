#include "scsi.h"

void scsi_fill_read10(SCSI_READ10 *cmd, int lun, unsigned long lba, unsigned short length)
{
	cmd->OpCode = SCSI_CMD_READ10;
	cmd->Flags.Value = 0;
	cmd->Flags.LUN = lun;
	cmd->LBA = HTOSCSI32(lba);
	cmd->Reserved = 0;
	cmd->TransferLength = HTOSCSI16(length);
	cmd->Control = 0;
}



