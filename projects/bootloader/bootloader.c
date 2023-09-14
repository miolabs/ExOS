#include <modules/bootloader.h>
#include <kernel/panic.h>
#include <kernel/machine/hal.h>

bool bootloader_check_image_tag(boot_tag_t *tag, boot_image_info_t *info)
{
	ASSERT(tag != nullptr, KERNEL_ERROR_NULL_POINTER);

	if (tag->Magic != BOOT_MAGIC_IMAGE)
		return false;

	unsigned load_addr = tag->Base;
	unsigned tag_offset = tag->Tag - load_addr;
	unsigned img_length = tag->End - load_addr;

	unsigned char *img_base = (unsigned char *)((unsigned)tag - tag_offset);
	if (((unsigned)img_base & 3) != 0)	// requires word aligmnent
		return false;

	// FIXME: move this check outside of this module
	unsigned *img_vectors = (unsigned *)img_base;
	if (img_vectors[13] == tag->Tag)
	{
		info->Start = img_base;
		info->Length = img_length;
		return true;
	}

	return false;
}

bool bootloader_search_image(unsigned **ptr, boot_image_info_t *info, unsigned search_size)
{
	ASSERT(ptr != nullptr, KERNEL_ERROR_NULL_POINTER);
	ASSERT(info != nullptr, KERNEL_ERROR_NULL_POINTER);

	unsigned *tag = (unsigned *)*ptr;
	if ((unsigned)tag >= search_size)
		return false;

	while(1)
	{
		if (*tag == BOOT_MAGIC_IMAGE)
		{
			if (bootloader_check_image_tag((boot_tag_t *)tag, info))
			{
				*ptr = tag;
				return true;
			}
		}

		tag++;
		if (0 == ((unsigned)tag & 0xFFF))
		{
			if ((unsigned)tag >= search_size)
			{
				*ptr = nullptr;
				return false;
			}
		}
	}

}

void bootloader_reboot(boot_image_info_t *info)
{
	ASSERT(info != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(info->Start != NULL, KERNEL_ERROR_NULL_POINTER);
	__machine_reboot(info->Start);
}

