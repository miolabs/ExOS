#ifndef EXOS_TREE_H
#define EXOS_TREE_H

#include <kernel/list.h>
#include <comm/comm.h>
#ifdef EXOS_FS_IO
#include <fs/block.h>
#endif

typedef enum
{
	EXOS_TREE_NODE_GROUP = 0,
	EXOS_TREE_NODE_DEVICE,
	EXOS_TREE_NODE_VOLUME,
} EXOS_TREE_NODE_TYPE;

typedef struct _TREE_GROUP EXOS_TREE_GROUP;

typedef struct
{
	EXOS_NODE Node;
	EXOS_TREE_GROUP *Parent;
	EXOS_TREE_NODE_TYPE Type;
	const char *Name;
} EXOS_TREE_NODE;

struct _TREE_GROUP
{
	EXOS_TREE_NODE;
	EXOS_LIST Children;
	EXOS_MUTEX Mutex;
};

typedef enum
{
	EXOS_TREE_DEVICE_COMM = 0,
	EXOS_TREE_DEVICE_BLOCK,
} EXOS_TREE_DEVICE_TYPE;

typedef struct
{
	EXOS_TREE_NODE;
	EXOS_TREE_DEVICE_TYPE DeviceType;
	union
	{
		COMM_DEVICE *Device;
#ifdef EXOS_FS_IO
		BLOCK_DEVICE *BlockDevice;
//		FS_VOLUME *Volume;
#endif
	};
	unsigned long Unit;
} EXOS_TREE_DEVICE;

void __tree_initialize();
void exos_tree_add_child(EXOS_TREE_GROUP *group, EXOS_TREE_NODE *child);
EXOS_TREE_NODE *exos_tree_find_path(EXOS_TREE_NODE *parent, const char *path);
EXOS_TREE_NODE *exos_tree_parse_path(EXOS_TREE_NODE *parent, const char **psubpath);
int exos_tree_add_child_path(EXOS_TREE_NODE *child, const char *parent_path);
void exos_tree_add_group(EXOS_TREE_GROUP *group, const char *parent_path);
void exos_tree_add_device(EXOS_TREE_DEVICE *device, const char *parent_path);

#endif // EXOS_TREE_H
