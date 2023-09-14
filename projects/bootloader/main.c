#include "board.h"
#include "terminal.h"
#include <modules/bootloader.h>
#include <kernel/memory.h>
#include <kernel/tree.h>
#include <usb/device.h>
#include <stdio.h>
#include <string.h>

static void _test(dispatcher_context_t *context, const char *cmd, void *state);
static void _mem(dispatcher_context_t *context, const char *cmd, void *state);
static void _dev(dispatcher_context_t *context, const char *cmd, void *state);
static terminal_cmd_t _cmds[] = {
	{ .Cmd = "test", .Func = _test }, 
	{ .Cmd = "mem", .Func = _mem },
	{ .Cmd = "dev", .Func = _dev },
	{ /* end */ } };
static void _prompt(const terminal_context_t *term);

static void _time_callback(dispatcher_context_t *context, dispatcher_t *dispatcher);

static boot_image_info_t _info;
static boot_tag_t *_tag = NULL;

void main()
{
	printf("\n\n\nreset!\n");
	dispatcher_context_t context;
	exos_dispatcher_context_create(&context);

	unsigned flash_size = board_flash_size();
	unsigned int *search_ptr = (unsigned int *)BOOT_OFFSET;
	if (bootloader_search_image(&search_ptr, &_info, flash_size))
	{
		board_led(BOARD_LEDF_RED);

		// NOTE: this is (probably) the running image
		printf("boot image found at $%x (%d bytes)\n", _info.Start, _info.Length);
		boot_tag_t *tag = (boot_tag_t *)search_ptr;
		printf("tag: '%s' ('%s' version=%d)\n", tag->Product, tag->Id, tag->Version);

		_tag = tag;
	}
	else
	{
		board_led(BOARD_LEDF_GREEN);

		printf("boot image not found\n");
	}

	static dispatcher_t _time_dispatcher;
	exos_dispatcher_create(&_time_dispatcher, NULL, _time_callback, NULL);
	exos_dispatcher_add(&context, &_time_dispatcher, 500);

//	usb_device_start();

	terminal_context_t term;
	terminal_context_create(&term, _cmds, _prompt, NULL);
	terminal_run(&term, &context, stdin, stdout);

	while(1)
	{
		exos_dispatch(&context, EXOS_TIMEOUT_NEVER);
	}
}

static void _time_callback(dispatcher_context_t *context, dispatcher_t *dispatcher)
{
	if (_tag != NULL)
	{
		board_led(0);
		
		printf("rebooting...");
		bootloader_reboot(&_info);
	}
}


static void _prompt(const terminal_context_t *term)
{
	_tag = NULL;	// disable reboot

	printf("$ ");
}

static void _test(dispatcher_context_t *context, const char *cmd, void *state)
{
	printf("test what?\n");
}

static void _mem(dispatcher_context_t *context, const char *cmd, void *state)
{
	unsigned avail = exos_mem_heap_avail();
	printf("heap has %d bytes free\n", avail);
}

static void _dir(const char *prefix, exos_tree_group_t *group)
{
	exos_mutex_lock(&group->Mutex);
	FOREACH(n, &group->Children)
	{
		exos_tree_node_t *item = (exos_tree_node_t *)n;
		const char *type_name;
		switch(item->Type)
		{
			case EXOS_TREE_NODE_GROUP:	type_name = "grp";	break;
			case EXOS_TREE_NODE_DEVICE:	type_name = "dev";	break;
			case EXOS_TREE_NODE_VOLUME:	type_name = "vol";	break;
			default:	type_name = "???";	break;
		}
		printf("%s/%s \t<%s>\n", prefix, item->Name, type_name);
		if (item->Type == EXOS_TREE_NODE_GROUP)
			_dir(item->Name, (exos_tree_group_t *)item);
	}
	exos_mutex_unlock(&group->Mutex);
}

static void _dev(dispatcher_context_t *context, const char *cmd, void *state)
{
	exos_tree_group_t *devs = exos_tree_find_group(NULL, "/dev");
	if (devs != NULL)
	{
		_dir("/dev", devs);
	}
	else printf("no devices found");
}

