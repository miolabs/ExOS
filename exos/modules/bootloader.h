#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <kernel/types.h>
#include <stdbool.h>

#define BOOT_MAKE_ID(a,b,c,d)	((a) | ((b) << 8) | ((c) << 16) | ((d) << 24)) 
#define BOOT_MAGIC_IMAGE BOOT_MAKE_ID('E', 'X', 'B', 'I')

typedef struct
{
	unsigned Magic;
	unsigned Base;
	unsigned Tag;
	unsigned End;
	unsigned Version;
	unsigned Checksum;
	const char *Product;
	const char *Id;
} boot_tag_t;

#define BOOTLOADER_IMAGE_TAG(base, end, prod, ver, id) __used const boot_tag_t __bootloader_tag = { .Magic = BOOT_MAGIC_IMAGE, \
	.Base = (unsigned)(base), .Tag = (unsigned)(&__bootloader_tag), .End = (unsigned)(end), \
	.Product = (const char *)(prod), .Version = (ver), .Id = (const char *)(id) };


typedef struct
{
	void *Start;
	unsigned Length;
} boot_image_info_t;

// prototypes
bool bootloader_check_image_tag(boot_tag_t *tag, boot_image_info_t *info);
bool bootloader_search_image(unsigned **ptr, boot_image_info_t *info, unsigned search_size);
void bootloader_reboot(boot_image_info_t *info) __noreturn;

#endif // BOOTLOADER_H

