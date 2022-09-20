#ifndef EXOS_TREE_H
#define EXOS_TREE_H

#include <kernel/list.h>
#include <kernel/mutex.h>
#include <stdbool.h>

typedef enum
{
	EXOS_TREE_NODE_GROUP = 0,
	EXOS_TREE_NODE_DEVICE,
	EXOS_TREE_NODE_VOLUME,
} exos_tree_node_type_t;

typedef struct __tree_group exos_tree_group_t;

typedef struct
{
	node_t Node;
	exos_tree_group_t *Parent;
	exos_tree_node_type_t Type;
	const char *Name;
} exos_tree_node_t;

struct __tree_group
{
	exos_tree_node_t;
	list_t Children;
	mutex_t Mutex;
};

void __tree_initialize();
void exos_tree_add_child(exos_tree_group_t *group, exos_tree_node_t *child);
exos_tree_group_t *exos_tree_find_group(exos_tree_group_t *parent, const char *path);
exos_tree_node_t *exos_tree_find_path(exos_tree_group_t *parent, const char *path);
exos_tree_node_t *exos_tree_parse_path(exos_tree_group_t *parent, const char **psubpath);
bool exos_tree_add_child_path(exos_tree_node_t *child, const char *parent_path);
void exos_tree_add_group(exos_tree_group_t *group, const char *parent_path);
int exos_tree_valid_name(const char *name);

#endif // EXOS_TREE_H
