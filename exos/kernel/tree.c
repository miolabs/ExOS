#include "tree.h"
#include <kernel/panic.h>

exos_tree_group_t __root = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Name = "root" };

static exos_tree_group_t _dev = {
#ifdef DEBUG
	.Node = { .Type = EXOS_NODE_TREE_NODE },
#endif
	.Type = EXOS_TREE_NODE_GROUP, .Name = "dev" };

static exos_tree_group_t _mnt = {
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

	exos_tree_add_child(&__root, (exos_tree_node_t *)&_dev);
	exos_tree_add_child(&__root, (exos_tree_node_t *)&_mnt);
}

void exos_tree_add_child(exos_tree_group_t *group, exos_tree_node_t *child)
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

exos_tree_group_t *exos_tree_find_group(exos_tree_group_t *parent, const char *path)
{
	if (path != NULL)
	{
		exos_tree_node_t *node = exos_tree_parse_path(parent, &path);
		return (node != NULL && node->Type == EXOS_TREE_NODE_GROUP) ? (exos_tree_group_t *)node : NULL; 
	}
	return parent;
}

exos_tree_node_t *exos_tree_find_path(exos_tree_group_t *parent, const char *path)
{
	ASSERT(path != NULL, KERNEL_ERROR_NULL_POINTER);
	exos_tree_node_t *node = exos_tree_parse_path(parent, &path);
	return (node != NULL && node->Type != EXOS_TREE_NODE_GROUP) ? node : NULL;
}

exos_tree_node_t *exos_tree_parse_path(exos_tree_group_t *parent, const char **psubpath)
{
	if (psubpath == NULL)
		kernel_panic(KERNEL_ERROR_NULL_POINTER);

	const char *subpath = *psubpath;
	if (*subpath == '/') 
	{
		subpath++;
		parent = &__root;
	}
	else if (parent == NULL) 
		parent = &__root;

	exos_tree_node_t *found = (exos_tree_node_t *)parent;
	if (subpath != NULL)
	{
		while(1)
		{
#ifdef DEBUG
			if (found->Node.Type != EXOS_NODE_TREE_NODE)
				kernel_panic(KERNEL_ERROR_LIST_CORRUPTED);
#endif
			if (*subpath != '\0' &&
				found->Type == EXOS_TREE_NODE_GROUP)
			{
				exos_tree_group_t *group = (exos_tree_group_t *)found;
				
				exos_mutex_lock(&group->Mutex);
				found = NULL;
				FOREACH(node, &group->Children)
				{
					exos_tree_node_t *child = (exos_tree_node_t *)node;
					if (_name_eq(&subpath, child->Name))
					{
						found = child;
						break;
					}
				}
				exos_mutex_unlock(&group->Mutex);

				if (found != NULL) 
					continue;
				
				found = (exos_tree_node_t *)group;
			}
			break;
		}
		
		*psubpath = subpath;
	}
	return found;
}

bool exos_tree_add_child_path(exos_tree_node_t *child, const char *parent_path)
{
	exos_tree_group_t *parent = exos_tree_find_group(NULL, parent_path);
	if (parent != NULL &&
		parent->Type == EXOS_TREE_NODE_GROUP)
	{
		exos_tree_add_child((exos_tree_group_t *)parent, child);
		return true;
	}
	return false;
}

void exos_tree_add_group(exos_tree_group_t *group, const char *parent_path)
{
	group->Type = EXOS_TREE_NODE_GROUP;
	list_initialize(&group->Children);
	exos_mutex_create(&group->Mutex);
	exos_tree_add_child_path((exos_tree_node_t *)group, parent_path);
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
