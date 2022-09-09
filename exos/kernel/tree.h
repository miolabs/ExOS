#ifndef EXOS_TREE_H
#define EXOS_TREE_H

#include <kernel/list.h>
#include <kernel/mutex.h>

typedef enum
{
	EXOS_TREE_NODE_GROUP = 0,
	EXOS_TREE_NODE_DEVICE,
	EXOS_TREE_NODE_VOLUME,
} EXOS_TREE_NODE_TYPE;

typedef struct _TREE_GROUP EXOS_TREE_GROUP;

typedef struct
{
	node_t Node;
	EXOS_TREE_GROUP *Parent;
	EXOS_TREE_NODE_TYPE Type;
	const char *Name;
} EXOS_TREE_NODE;

struct _TREE_GROUP
{
	EXOS_TREE_NODE;
	list_t Children;
	mutex_t Mutex;
};

void __tree_initialize();
void exos_tree_add_child(EXOS_TREE_GROUP *group, EXOS_TREE_NODE *child);
EXOS_TREE_NODE *exos_tree_find_group(EXOS_TREE_NODE *parent, const char *path);
EXOS_TREE_NODE *exos_tree_find_path(EXOS_TREE_NODE *parent, const char *path);
EXOS_TREE_NODE *exos_tree_parse_path(EXOS_TREE_NODE *parent, const char **psubpath);
int exos_tree_add_child_path(EXOS_TREE_NODE *child, const char *parent_path);
void exos_tree_add_group(EXOS_TREE_GROUP *group, const char *parent_path);
int exos_tree_valid_name(const char *name);

#endif // EXOS_TREE_H
