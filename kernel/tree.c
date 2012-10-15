#include "tree.h"
#include <kernel/panic.h>

static EXOS_LIST _root_children;
static EXOS_LIST _dev_children;
static EXOS_LIST _mnt_children;

const EXOS_TREE_GROUP __root = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_root_children, .Name = "root" };

static EXOS_TREE_GROUP _dev = {
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_dev_children, .Name = "dev" };

static EXOS_TREE_GROUP _mnt = {
	.Type = EXOS_TREE_NODE_GROUP, .Children = &_mnt_children, .Name = "mnt" };

void __tree_initialize()
{
	list_initialize(&_root_children);
	list_initialize(&_dev_children);
	list_initialize(&_mnt_children);

	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_dev);
	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_mnt);
}

void exos_tree_add_child(const EXOS_TREE_GROUP *group, EXOS_TREE_NODE *child)
{
#ifdef DEBUG
	if (child == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	child->Node.Type = EXOS_NODE_TREE_NODE;
#endif
	list_add_tail(group->Children, (EXOS_NODE *)child);
}

void exos_tree_add_device(EXOS_TREE_DEVICE *device)
{
	exos_tree_add_child(&_dev, (EXOS_TREE_NODE *)device);
}

static int _name_eq(const char **ppath, const char *name)
{
	const char *path = *ppath;
	char c;
	while((c = *name++) && (c == *path++));
	if (c == '\0')
	{
		if (*path == '\0')
		{
			*ppath = path;
			 return 1;
		}
		else if (*path == '/')
		{
			*ppath = ++path;
			return 1;
		}
	}
	return 0;
}

EXOS_TREE_NODE *exos_tree_find_node(EXOS_TREE_NODE *parent, const char **ppath)
{
	if (parent == NULL) parent = (EXOS_TREE_NODE *)&__root;

	do
	{
#ifdef DEBUG
		if (parent->Node.Type != EXOS_NODE_TREE_NODE)
			kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
		if (parent->Type == EXOS_TREE_NODE_GROUP)
		{
			EXOS_TREE_GROUP *group = (EXOS_TREE_GROUP *)parent;
			// TODO: lock children
			EXOS_TREE_NODE *found = NULL;
			FOREACH(node, group->Children)
			{
				EXOS_TREE_NODE *child = (EXOS_TREE_NODE *)node;
				if (_name_eq(ppath, child->Name))
				{
					found = child;
					break;
				}
			}
			parent = found;
		}
		else 
		{
			break;
		}
	} while (parent != NULL);
	return parent;
}

