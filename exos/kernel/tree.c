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
	ASSERT(group != NULL && child != NULL, KERNEL_ERROR_NULL_POINTER);
	ASSERT(group->Type == EXOS_TREE_NODE_GROUP, KERNEL_ERROR_WRONG_NODE);
	ASSERT(child->Name != NULL, KERNEL_ERROR_NULL_POINTER);

	exos_mutex_lock(&group->Mutex);
#ifdef DEBUG
	if (list_find_node(&group->Children, (node_t *)child))
		kernel_panic(KERNEL_ERROR_LIST_ALREADY_CONTAINS_NODE);

	child->Node.Type = EXOS_NODE_TREE_NODE;
#endif
	list_add_tail(&group->Children, (node_t *)child);
	exos_mutex_unlock(&group->Mutex);
	child->Parent = group;
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

EXOS_TREE_NODE *exos_tree_find_group(EXOS_TREE_NODE *parent, const char *path)
{
	if (path != NULL)
	{
		EXOS_TREE_NODE *node = exos_tree_parse_path(parent, &path);
		return (node != NULL && node->Type == EXOS_TREE_NODE_GROUP) ? node : NULL; 
	}
	return parent;
}

EXOS_TREE_NODE *exos_tree_find_path(EXOS_TREE_NODE *parent, const char **ppath)
{
	ASSERT(ppath != NULL, KERNEL_ERROR_NULL_POINTER);
	const char *path = *ppath;
	EXOS_TREE_NODE *node = exos_tree_parse_path(parent, ppath);
	if (node != NULL && node->Type != EXOS_TREE_NODE_GROUP)
	{
		*ppath = path;
		return node;
	}
	return NULL; 
}

EXOS_TREE_NODE *exos_tree_parse_path(EXOS_TREE_NODE *parent, const char **psubpath)
{
	if (psubpath == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	const char *subpath = *psubpath;
	if (*subpath == '/') 
	{
		subpath++;
		parent = (EXOS_TREE_NODE *)&__root;
	}
	else if (parent == NULL) 
		parent = (EXOS_TREE_NODE *)&__root;

	if (subpath != NULL)
	{
		while(1)
		{
#ifdef DEBUG
			if (parent->Node.Type != EXOS_NODE_TREE_NODE)
				kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
			if (*subpath != '\0' &&
				parent->Type == EXOS_TREE_NODE_GROUP)
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

				if (found != NULL) 
				{
					parent = found;
					continue;
				}	
			}
			break;
		}
		
		*psubpath = subpath;
	}
	return parent;
}

int exos_tree_add_child_path(EXOS_TREE_NODE *child, const char *parent_path)
{
	EXOS_TREE_NODE *parent = exos_tree_find_group(NULL, parent_path);
	if (parent != NULL &&
		parent->Type == EXOS_TREE_NODE_GROUP)
	{
		exos_tree_add_child((EXOS_TREE_GROUP *)parent, child);
		return 1;
	}
	return 0;
}

void exos_tree_add_group(EXOS_TREE_GROUP *group, const char *parent_path)
{
	group->Type = EXOS_TREE_NODE_GROUP;
	list_initialize(&group->Children);
	exos_mutex_create(&group->Mutex);
	exos_tree_add_child_path((EXOS_TREE_NODE *)group, parent_path);
}

int exos_tree_valid_name(const char *name)
{
	int length = 0;
	char c;
	while(c = name[length], c != '\0')
	{
		if ((c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c == '_')) 
		{
			length++;
			continue;
		}
		return 0;
	}
	return length;
}
