#include "tree.h"

static EXOS_LIST _root_children;
static EXOS_LIST _dev_children;
static EXOS_LIST _mnt_children;

const EXOS_TREE_GROUP __root = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_root_children, .Name = "root" };

static EXOS_TREE_GROUP _dev = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_dev_children, .Name = "dev" };

static EXOS_TREE_GROUP _mnt = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_mnt_children, .Name = "mnt" };

void __tree_initialize()
{
	list_initialize(&_root_children);
	list_initialize(&_dev_children);
	list_initialize(&_mnt_children);

	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_dev);
	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_mnt);
}

void exos_tree_add_child(const EXOS_TREE_GROUP *group, const EXOS_TREE_NODE *child)
{
	list_add_tail(group->Children, (EXOS_NODE *)child);
}

