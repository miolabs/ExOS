#include <support/misc/sdcard/sdcard.h>
#include <kernel/machine/hal.h>
#include <fs/file.h>
#include <fs/rawfs.h>

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

int _get_media_info(RAWFS_MEDIA_INFO *info)
{
	SD_INFO sd_info;
	sd_get_info(&sd_info);
	*info = (RAWFS_MEDIA_INFO) { .Blocks = sd_info.Blocks, .BlockSize = sd_info.BlockSize };
	return 1;
}

void * _alloc_buffer()
{
	return NULL;
}

void _free_buffer(void *buffer)
{
}

int _read(void *buffer, unsigned long block, unsigned long count)
{
}

int _write(void *buffer, unsigned long block, unsigned long count)
{
}

EXOS_TREE_VOLUME _tree_vol;
static FS_VOLUME _vol;
static RAWFS_CONTEXT _context;
static const RAWFS_DRIVER _sd_driver = {
	.GetMediaInfo = _get_media_info,
	.AllocBuffer = _alloc_buffer,
	.FreeBuffer = _free_buffer,
	.Read = _read,
	.Write = _write };

void main()
{
	int done = sd_initialize();
	if (done)
		done = rawfs_create(&_vol, &_context, &_sd_driver);
	if (done)
		done = fs_mount_volume(&_vol, &_tree_vol, "/dev/sdcard0");

	FILE_IO_ENTRY file;
	done = file_open(&file, "/dev/sdcard0", FS_LOCKF_WRITE);	// fails due to lack of kernel memory (is heap added?)

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

