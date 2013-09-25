#include <support/misc/sdcard/sdcard.h>
#include <kernel/machine/hal.h>
#include <fs/filestream.h>

#define BUFFER_BLOCKS 24

typedef unsigned char BLOCK[512];
 
static BLOCK _buffers[BUFFER_BLOCKS] __dma;

static void _init_block(BLOCK *buf, unsigned long index)
{
	unsigned long *ptr = (unsigned long *)buf;
	int size = sizeof(BLOCK) / sizeof(unsigned long);
	for (int i = 0; i < size; i++) ptr[i] = index;
	ptr[0] = 0xFF5555AA;
}

FS_VOLUME _vol;

void main()
{
	int done = sd_initialize();

	FILE_IO_ENTRY file;
	done = file_open(&file, "/dev/sdcard0", FS_LOCKF_WRITE);

	SD_INFO info;
	sd_get_info(&info);
	unsigned long long size = info.BlockSize * info.Blocks;
	unsigned long blocks = size / sizeof(BLOCK);

	SD_ERROR err = SD_OK;
	int bl = 0;
	while(bl < blocks)
	{
		int count = (bl + BUFFER_BLOCKS) > blocks ? blocks - bl : BUFFER_BLOCKS;
		for (int i = 0; i < count; i++)
			_init_block(&_buffers[i], bl + i);
		err = sd_write_blocks(bl, count, (unsigned char *)_buffers);
		if (err != SD_OK) break;
		bl += count;
	}
}

