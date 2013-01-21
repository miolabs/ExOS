#include "tree.h"
#include <kernel/panic.h>

EXOS_TREE_GROUP __root = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Name = "root" };

static EXOS_TREE_GROUP _dev = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Name = "dev" };

static EXOS_TREE_GROUP _mnt = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Name = "mnt" };

void __tree_initialize()
{
	list_initialize(&__root.Children);
	exos_mutex_create(&__root.Mutex);
	list_initialize(&_dev.Children);
	exos_mutex_create(&_dev.Mutex);
	list_initialize(&_mnt.Children);
	exos_mutex_create(&_mnt.Mutex);

	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_dev);
	exos_tree_add_child(&__root, (EXOS_TREE_NODE *)&_mnt);
}

void exos_tree_add_child(EXOS_TREE_GROUP *group, EXOS_TREE_NODE *child)
{
#ifdef DEBUG
	if (child == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);
	child->Node.Type = EXOS_NODE_TREE_NODE;
#endif
	exos_mutex_lock(&group->Mutex);
	list_add_tail(&group->Children, (EXOS_NODE *)child);
	exos_mutex_unlock(&group->Mutex);
}

void exos_tree_add_device(EXOS_TREE_DEVICE *device)
{
	device->Type = EXOS_TREE_NODE_DEVICE;
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

EXOS_TREE_NODE *exos_tree_find_node(EXOS_TREE_NODE *parent, const char *path)
{
	if (parent == NULL) parent = (EXOS_TREE_NODE *)&__root;

	const char *subpath = path;
	do
	{
#ifdef DEBUG
		if (parent->Node.Type != EXOS_NODE_TREE_NODE)
			kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
		if (parent->Type == EXOS_TREE_NODE_GROUP)
		{
			EXOS_TREE_GROUP *group = (EXOS_TREE_GROUP *)parent;
			
			exos_mutex_lock(&group->Mutex);
			EXOS_TREE_NODE *found = NULL;
			FOREACH(node, &group->Children)
			{
				EXOS_TREE_NODE *child = (EXOS_TREE_NODE *)node;
				if (_name_eq(&subpath, child->Name))
				{
					found = child;
					break;
				}
			}
			exos_mutex_unlock(&group->Mutex);

			parent = found;
		}
		else 
		{
			break;
		}
	} while (parent != NULL);
	return parent;
}

