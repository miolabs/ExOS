#ifndef EXOS_TREE_H
#define EXOS_TREE_H

#include <kernel/list.h>
#include <comm/comm.h>

typedef enum
{
	EXOS_TREE_NODE_GROUP = 0,
	EXOS_TREE_NODE_DEVICE,
	EXOS_TREE_NODE_MOUNT_POINT,
} EXOS_TREE_NODE_TYPE;

typedef struct _TREE_NODE
{
	EXOS_NODE Node;
	struct _TREE_NODE *Parent;
	EXOS_TREE_NODE_TYPE Type;
	const char *Name;
} EXOS_TREE_NODE;

typedef struct
{
	EXOS_TREE_NODE;
	EXOS_LIST *Children;
} EXOS_TREE_GROUP;

typedef struct
{
	EXOS_TREE_NODE;
	COMM_DEVICE *Device;
	unsigned long Port;
} EXOS_TREE_DEVICE;

void __tree_initialize();
void exos_tree_add_child(const EXOS_TREE_GROUP *group, EXOS_TREE_NODE *child);
void exos_tree_add_device(EXOS_TREE_DEVICE *device);
EXOS_TREE_NODE *exos_tree_find_node(EXOS_TREE_NODE *parent, const char *path);

#endif // EXOS_TREE_H
