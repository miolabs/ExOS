#ifndef EXOS_TREE_H
#define EXOS_TREE_H

#include <kernel/list.h>
#include <kernel/io.h>

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
} EXOS_TREE_NODE;

typedef struct
{
	EXOS_TREE_NODE;
	EXOS_LIST *Children;
	char *Name;
} EXOS_TREE_GROUP;


void __tree_initialize();
void exos_tree_add_child(const EXOS_TREE_GROUP *group, const EXOS_TREE_NODE *child);

#endif // EXOS_TREE_H
